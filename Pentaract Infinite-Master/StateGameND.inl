static StateGameND* instance() { static StateGameND game; return &game; }

void configure(const std::filesystem::path& worldPath, uint64_t seed, bool flat, bool caves)
{
    path=worldPath;playerSpeed=7.;rayGroup={};axisGroups.reset(N);configuringAxes=false;
    renderType=3;
    worldResolution.percent=100;
    if(std::filesystem::exists(path/"info.json")) {
        auto info=nlohmann::json::parse(std::ifstream(path/"info.json"));
        renderType=worldRenderType(N,info.value("renderType",3));
    }
    controlBank=0;bankKeyHeld=false;selectedHidden=renderType==2?1:2;sideControls={};sidePlacement.reset();sideWalkTime=0;sideWalkDirection=0;
    generation=std::make_shared<Generation>(seed,caves,flat);
    auto library=structures::library("Structure");
    std::filesystem::create_directories(library);
    generation->load(library);
    streamValid=false;viewWanted.clear();viewColumns.clear();
    viewSnapshot=std::make_shared<const std::set<Position>>();viewRevision=0;
    auto gen=generation;
    temporal.reset();
    logic.reset();
    space=std::make_unique<Space>([gen](ColumnChunk& c,const Position& p){(*gen)(c,p);});
    space->setCacheLimit(8192);
    viewTask={};meshViewDirty=true;
    requestCursor=0;
    pendingUploads.clear();renderedMasks.clear();
    logic=std::make_unique<Logic>(*space,[this](const Position& p){return generation->faces(p);});
    eye.fill(.5);eye[0]=double(generation->terrain.height({}))+2.8;
    plane={1,2}; yaw=0; pitch=0; verticalSpeed=0; flight=false; noclip=false;
    frame.align(plane);middleMouse=freePlane=false;
    regions.clear(); excluded.clear(); falling={}; water={}; registered.clear(); wanted.clear(); rendered.clear();
    selected=Block::GRASS; firstAxis=-1; fallTimer=0; structureJob=StructureJob::None; placing.reset();
    readSave();constrainGroupFrame();logic->resumeCircuits();
    for(const auto& [p,id]:space->modifications())if(id==ExtendedBlock::WATER)water.changed(p,id);
    auto temporalFile=path/("temporal"+std::to_string(N)+"d.bin");
    auto info=std::filesystem::exists(path/"info.json")?nlohmann::json::parse(std::ifstream(path/"info.json")):nlohmann::json::object();
    if(info.value("temporalDimensions",std::filesystem::exists(temporalFile)?2:1)==2) {
        temporal=std::make_unique<Temporal>(*space,*logic,falling);
        temporal->load(temporalFile);
    }
    frameMeter={};
    space->setResolver([this](const Position&) {
        auto gen=generation;
        return [gen](ColumnChunk& c,const Position& p){(*gen)(c,p);};
    });
}

void readSave()
{
    auto binary=path/("blocks"+std::to_string(N)+"d.bin");
    if (std::filesystem::exists(binary))
    {
        std::ifstream in(binary,std::ios::binary); char magic[8]{}; in.read(magic,8);
        if(std::string(magic,8)!=worldMagic() || structures::binary::get(in,2)!=Engine::saveVersion)
            throw std::runtime_error("Invalid ND world save");
        if constexpr(N>15)if(structures::binary::get(in,4)!=N)throw std::runtime_error("Wrong world dimension");
        uint64_t count=structures::binary::get(in,8);
        if(count>(UINT64_MAX-worldHeaderBytes)/blockRecordBytes || std::filesystem::file_size(binary)!=worldHeaderBytes+count*blockRecordBytes)
            throw std::runtime_error("Truncated ND world save");
        for(uint64_t i=0;i<count;++i)
        {
            Position p{}; for(auto& v:p) v=int32_t(uint32_t(structures::binary::get(in,4)));
            BlockState value{uint16_t(structures::binary::get(in,2))};
            for(auto& word:value.faces.words)word=uint32_t(structures::binary::get(in,4));
            validate(value); space->restore(p,value.id);logic->restoreCircuit(p,value.id);
            if(ExtendedBlock::isRedirector(value.id)) logic->restoreFaces(p,value.faces);
        }
    }
    if(std::filesystem::exists(path/("player"+std::to_string(N)+"d.json")))
    {
        nlohmann::json j; std::ifstream(path/("player"+std::to_string(N)+"d.json"))>>j;
        eye=j.at("position").get<Vector>();
        if(j.contains("axisGroups"))axisGroups.assign(j.at("axisGroups").get<std::vector<std::vector<int>>>(),N);
        controlBank=std::clamp(j.value("controlBank",0),0,int(axisGroups.groups.size())-1);
        int resolution=j.value("worldResolution",100);
        worldResolution.percent=resolution==50 || resolution==75?resolution:100;
        for(double n:eye) if(!std::isfinite(n) || n<INT32_MIN+1024. || n>INT32_MAX-1024.) throw std::runtime_error("Invalid ND player coordinate");
        plane=j.value("plane",std::array<int,2>{1,2});
        if(plane[0]<1 || plane[1]<1 || plane[0]>(N-1) || plane[1]>(N-1) || plane[0]==plane[1]) plane={1,2};
        if(renderType==2)SideViewND::align<N>(frame.axes,plane[0]);else frame.align(plane);
        if(j.contains("cameraFrame")) {
            // The full basis grows quadratically; only individual rows belong on the stack.
            auto saved=j.at("cameraFrame").get<std::vector<Vector>>();
            bool valid=saved.size()==N-1;
            for(int a=0;valid && a<(N-1);++a) {
                for(double n:saved[a]) valid=valid && std::isfinite(n);
                valid=valid && std::abs(saved[a][0])<1e-8;
                for(int b=0;b<(N-1);++b) {
                    double dot=0;for(int i=1;i<N;++i)dot+=saved[a][i]*saved[b][i];
                    valid=valid && std::abs(dot-(a==b?1.:0.))<1e-6;
                }
            }
            if(valid) {std::copy(saved.begin(),saved.end(),frame.axes.begin());freePlane=j.value("freePlane",false);}
        }
        yaw=j.value("yaw",0.); pitch=j.value("pitch",0.);
        if(!std::isfinite(yaw) || !std::isfinite(pitch)) {yaw=0;pitch=0;}
        pitch=std::clamp(pitch,-1.55,1.55);
        flight=j.value("flight",false); noclip=j.value("noclip",false);
        selected=j.value<uint16_t>("selected",Block::GRASS);
        if(!ExtendedBlock::isRenderable(selected)) selected=Block::GRASS;
        excluded=j.value("excluded",std::set<uint16_t>{});
        selectedHidden=bankHidden(0);
        for(const auto& r:j.value("regions",nlohmann::json::array()))
        {
            Region region{r.at("offset").get<Position>(),r.at("size").get<Position>(),r.at("name").get<std::string>()};
            for(int n:region.size) if(n<1) throw std::runtime_error("Invalid saved structure region");
            regions[r.at("owner").get<Position>()]=std::move(region);
        }
    }
}

void save()
{
    if(!space || path.empty()) return;
    std::filesystem::create_directories(path);
    if(temporal)temporal->save(path/("temporal"+std::to_string(N)+"d.bin"));
    auto info=std::filesystem::exists(path/"info.json")?nlohmann::json::parse(std::ifstream(path/"info.json")):nlohmann::json::object();
    info["temporalDimensions"]=temporal?2:1;
    {std::ofstream out(path/"info.json");out<<info.dump(2);if(!out)throw std::runtime_error("Cannot save temporal settings");}
    auto temp=path/("blocks"+std::to_string(N)+"d.bin.tmp");
    {
        std::ofstream out(temp,std::ios::binary|std::ios::trunc);
        out.write(worldMagic().data(),8); structures::binary::put(out,Engine::saveVersion,2);
        if constexpr(N>15)structures::binary::put(out,N,4);
        structures::binary::put(out,space->modifications().size(),8);
        for(const auto& [p,id]:space->modifications())
        {
            for(int n:p) structures::binary::put(out,uint32_t(n),4);
            structures::binary::put(out,id,2);
            auto it=logic->faceEdits().find(p);
            auto mask=it==logic->faceEdits().end()?FaceMask{}:it->second;
            for(auto word:mask.words)structures::binary::put(out,word,4);
        }
        out.flush(); if(!out) throw std::runtime_error("Cannot save ND world");
    }
    replace(temp,path/("blocks"+std::to_string(N)+"d.bin"));
    nlohmann::json selections=nlohmann::json::array();
    for(const auto& [p,r]:regions) selections.push_back({{"owner",p},{"offset",r.offset},{"size",r.size},{"name",r.name}});
    jsonFile(path/("player"+std::to_string(N)+"d.json"),{{"position",eye},{"plane",plane},{"yaw",yaw},{"pitch",pitch},
        {"cameraFrame",frame.axes},{"freePlane",freePlane},{"axisGroups",axisGroups.groups},{"controlBank",controlBank},{"worldResolution",worldResolution.percent},
        {"flight",flight},{"noclip",noclip},{"selected",selected},{"excluded",excluded},{"regions",selections}});
    // Retire only obsolete history files, after the world edits/player save succeeded.
    std::error_code ignored;
    std::filesystem::remove(path/("generation"+std::to_string(N)+"d.bin"),ignored);
    std::filesystem::remove(path/("generation"+std::to_string(N)+"d.bin.tmp"),ignored);
}

void reloadStructures()
{
    auto root=structures::library("Structure");
    auto next=std::make_shared<Generation>(*generation);next->load(root);
    // In-flight workers keep their immutable snapshot; future requests use these rules.
    generation=next;space->clearGenerationCache();save();
}

void init(StateManager& s)
{
    if(renderType==2)for(const auto* name:{"idle","walk1","walk2-left","walk2-right","jump"})
        if(!Texture::get(std::string("player2d.")+name) && !Texture::load(std::string("assets/textures/player2d/")+name+".png",4,false,std::string("player2d.")+name))
            throw std::runtime_error("Cannot load 2D player sprite");
    if(!Shader::get(("renderer"+std::to_string(N)+"d")) && !Shader::load(("renderer"+std::to_string(N)+"d"),{{GL_VERTEX_SHADER,"assets/shaders/renderer.vert"},
        {GL_FRAGMENT_SHADER,"assets/shaders/rendererND.glsl"}},N)) throw std::runtime_error("Renderer compilation failed ("+dimName()+"). See shader-errors.log.");
    shader=Shader::get(("renderer"+std::to_string(N)+"d"));
    mesh.clear();
    if(!Shader::get(("sliceMesh"+std::to_string(N)+"d")) && !Shader::load(("sliceMesh"+std::to_string(N)+"d"),{
        {GL_VERTEX_SHADER,"assets/shaders/sliceMesh.vert"},{GL_FRAGMENT_SHADER,"assets/shaders/sliceMeshND.glsl"}},N))
        throw std::runtime_error("Mesh shader compilation failed ("+dimName()+"). See shader-errors.log.");
    meshShader=Shader::get(("sliceMesh"+std::to_string(N)+"d"));
    std::vector<std::string> names{"dirt","grassSide","grassTop","stone","woodSide","woodTop","glass","bricks","water",
        "custom","deepstone","antiSand","leverOff","leverOn"};
    for(int i=0;i<16;++i) names.push_back("cable"+std::to_string(i));
    names.push_back("crystalDoor"); names.push_back("crystalDoorOpen");
    for(int i=0;i<16;++i) names.push_back("repeater"+std::to_string(i));
    names.push_back("redirectorOff"); names.push_back("redirectorOn");
    for(int i=0;i<10;++i) names.push_back("leaf"+std::to_string(i));
    std::vector<uint64_t> textureHandles;
    std::vector<uint8_t> textureOpaque;
    for(const auto& name:names)
    {
        std::string key="blocks."+name;
        if(!Texture::get(key)) Texture::load("assets/textures/blocks/"+name+".ndtf",4,false,key,true);
        auto t=Texture::get(key); if(!t) throw std::runtime_error("Missing block texture: "+name);
        textureHandles.push_back(t->getHandle());
        int width=0,height=0,depth=0;
        glGetTextureLevelParameteriv(t->id(),0,GL_TEXTURE_WIDTH,&width);
        glGetTextureLevelParameteriv(t->id(),0,GL_TEXTURE_HEIGHT,&height);
        glGetTextureLevelParameteriv(t->id(),0,GL_TEXTURE_DEPTH,&depth);
        std::vector<uint8_t> pixels(size_t(width)*height*std::max(depth,1)*4);
        glGetTextureImage(t->id(),0,GL_RGBA,GL_UNSIGNED_BYTE,GLsizei(pixels.size()),pixels.data());
        bool opaque=!pixels.empty();
        for(size_t i=3;i<pixels.size();i+=4)if(pixels[i]==0){opaque=false;break;}
        textureOpaque.push_back(opaque);
    }
    tiles=GPUBuffer(textureHandles.size()*8,textureHandles.data());
    std::vector<int> table(ExtendedBlock::COUNT*(2*N),0);
    selectable.clear();
    for(int id=1;id<ExtendedBlock::COUNT;++id)
    {
        if(!ExtendedBlock::isRenderable(id)) continue;
        if(id<Block::COUNT || ExtendedBlock::isSelectable(id)) selectable.push_back((uint16_t)id);
        for(int side=0;side<(2*N);++side)
            table[id*(2*N)+side]=id<Block::COUNT?Block::blocks[id].textures[side<10?side:8+side%2]:ExtendedBlock::data(id).textures[side<10?side:8+side%2];
    }
    textures=GPUBuffer(table.size()*sizeof(int),table.data());
    std::vector<uint8_t> blockOpaque(ExtendedBlock::COUNT,0);
    for(int id=1;id<ExtendedBlock::COUNT;++id)if(ExtendedBlock::isRenderable(id) && id!=ExtendedBlock::CRYSTAL_DOOR_OPEN) {
        bool opaque=true;
        for(int side=0;side<(2*N);++side)opaque=opaque && textureOpaque.at(table[id*(2*N)+side]);
        if(ExtendedBlock::isRedirector(id))opaque=opaque && textureOpaque.at(48) && textureOpaque.at(49);
        blockOpaque[id]=opaque;
    }
    mesh.setOpaque(std::move(blockOpaque));
    handles=GPUBuffer(8); masks=GPUBuffer(32);
    uint32_t zero=0; lookup=GPUBuffer(4,&zero); lookupDirty=true;
    const std::pair<const char*,const char*> icons[]{
        {"blockIcons.png","blockIcons.png"},{"deepstoneIcons.png","extended.deepstoneIcon"},
        {"antiSandIcons.png","extended.antiSandIcon"},{"leverIcons.png","extended.leverIcon"},
        {"cableIcons.png","extended.cableIcon"},{"crystalDoorIcons.png","extended.crystalDoorIcon"},
        {"reapeaterIcons.png","extended.repeaterIcon"},{"redirectorIcons.png","extended.redirectorIcon"},
        {"StructureBlockIcon.png","extended.structureIcon"},{"leafIcon.png","extended.leafIcon"}};
    for(const auto& [file,key]:icons)
        if(!Texture::get(key) && !Texture::load(std::string("assets/textures/")+file,4,false,key))
            throw std::runtime_error(std::string("Missing block icon: ")+file);
    hud.clear(); coords.setSize(2); coords.setShadow(true);coords.setOffsetX(12); coords.setOffsetY(12); hud.addElem(&coords);
    selection.setSize(2); selection.setAlignX(ui::ALIGN_RIGHT); selection.setAlignY(ui::ALIGN_BOTTOM);
    selection.setOffsetX(-20); selection.setOffsetY(-96); hud.addElem(&selection);
    blockIcon(selectedIcon,selected,64);
    selectedIcon.setAlignX(ui::ALIGN_RIGHT);selectedIcon.setAlignY(ui::ALIGN_BOTTOM);
    selectedIcon.setOffsetX(-20);selectedIcon.setOffsetY(-20);hud.addElem(&selectedIcon);
    hud.init(s.getWindow()); panel=QuadRenderer(Shader::get("quad")); panel.init();
    console.init(s.getWindow());console.execute=[this](std::string text){return command(std::move(text));};
    menu=inventory=editing=false; keys.clear(); cursor(s,false); camera(); stream();
    int w,h; s.getSize(&w,&h); windowResize(s,w,h);
}

void close(StateManager& s)
{
    try {save();} catch(const std::exception& e) {MessageBoxA(nullptr,e.what(),"ND save failed",MB_OK|MB_ICONERROR);}
    console.cleanup();
    cursor(s,true); s.setUiPage(nullptr); rendered.clear(); registered.clear();panel.cleanup();
    handles.cleanup(); textures.cleanup(); tiles.cleanup(); masks.cleanup(); lookup.cleanup();frameBuffer.cleanup();
    worldResolution.cleanup();
    mesh.clear();
}

void camera()
{
    if(renderType==2){right=frame.axes[0];forward=frame.axes[1];up={};up[0]=1;return;}
    forward={}; right={}; up={};
    forward[0]=std::sin(pitch); up[0]=std::cos(pitch);
    auto heading=frame.heading(yaw);right=frame.right(yaw);
    for(int i=1;i<N;++i) {forward[i]=heading[i]*std::cos(pitch);up[i]=-heading[i]*std::sin(pitch);}
}

void stream()
{
    auto center=address(cell(eye)).chunk;
    bool published=space->collect();
    auto newColumns=space->takePublished();
    bool planned=false;
    ViewPlan ready;
    if(viewTask.valid() && viewTask.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
        ready=viewTask.get();viewColumns=std::move(ready.columns);streamEye=plannedEye;streamFrame=plannedFrame;
        streamUnderground=streamEye[0]<8;streamValid=true;planned=true;
    }
    auto aligned=SliceColumns::alignedAxes(frame.axes[0],frame.axes[1]);
    bool rebuild=!streamValid || streamFrame.axes!=frame.axes || streamUnderground!=(eye[0]<8);
    if(renderType==2)for(double margin:{-11.,11.})
        rebuild|=std::floor((eye[0]+margin)/32.)!=std::floor((streamEye[0]+margin)/32.);
    // Keep a padded view resident across small visible-axis movements.
    const double moveThreshold=renderType!=2 && aligned[0]>=0?2.:.5;
    for(int i=1;i<N;++i)rebuild|=std::abs(eye[i]-streamEye[i])>moveThreshold;
    if(aligned[0]<0)for(int hidden=2;hidden<(N-1);++hidden) {
        double offset=0;
        for(int i=1;i<N;++i)offset+=(eye[i]-streamEye[i])*frame.axes[hidden][i];
        rebuild|=std::abs(offset)>1e-6;
    }
    if(aligned[0]>=0)for(int i=1;i<N;++i)
        if(i!=aligned[0] && i!=aligned[1])rebuild|=std::floor(eye[i])!=std::floor(streamEye[i]);
    if(renderType==2)for(int i=1;i<N;++i)rebuild|=std::abs(eye[i]-streamEye[i])>1e-8;
    // Apply a completed plan before capturing the base of its successor.
    if(rebuild && !viewTask.valid() && !planned) {
        plannedEye=eye;plannedFrame=frame;
        viewTask=BackgroundWork::rendering().addTaskFuture([p=eye,x=frame.axes[0],z=frame.axes[1],side=renderType==2,previous=viewSnapshot,revision=viewRevision] {
            auto aligned=SliceColumns::alignedAxes(x,z);
            ViewPlan result;
            result.columns=side?SideViewND::columns(p,x,16.):SliceColumns::enumerate(p,x,z,(p[0]<8?13.:21.)+(aligned[0]>=0?2.:0.));
            int a=int(std::floor(p[0]/32.));result.lowA=a-2;result.highA=a+2;
            if(p[0]>=8)result.lowA=std::max(result.lowA,0);
            if(side){result.lowA=int(std::floor((p[0]-11.)/32.));result.highA=int(std::floor((p[0]+11.)/32.));}
            std::vector<std::pair<double,Position>> ranked;
            for(auto key:result.columns)for(int a=result.lowA;a<=result.highA;++a) {
                key[0]=a;result.view.insert(key);
                double distance=0;for(int axis=0;axis<N;++axis) {
                    double delta=(double(key[axis])+.5)*extent[axis]-p[axis];distance+=delta*delta;
                }
                ranked.emplace_back(distance,key);
            }
            std::sort(ranked.begin(),ranked.end());result.order.reserve(ranked.size());
            for(const auto& [distance,key]:ranked)result.order.push_back(key);
            result.baseRevision=revision;
            if(previous) {
                std::set_difference(result.view.begin(),result.view.end(),previous->begin(),previous->end(),std::back_inserter(result.added));
                std::set_difference(previous->begin(),previous->end(),result.view.begin(),result.view.end(),std::back_inserter(result.removed));
            }
            result.snapshot=std::make_shared<const std::set<Position>>(result.view);
            return result;
        });
    }
    auto collisionCore=renderType==2?SideViewND::columns(eye,frame.axes[0],1.25):SliceColumns::enumerate(eye,frame.axes[0],frame.axes[1],1.25);
    const auto& collisionRegion=collisionPrefetch.update(std::move(collisionCore),int(std::floor((eye[0]-2.65)/32.)),
        int(std::floor((eye[0]+1.1)/32.)),rayGroup.axes);
    bool verticalChanged=center[0]!=address(cell(streamPassEye)).chunk[0];
    bool reorder=planned || verticalChanged || streamDirty || editing || structureJob!=StructureJob::None;
    bool fullResidency=!regions.empty() || streamDirty || editing || structureJob!=StructureJob::None;
    bool residencyChanged=reorder || collisionRegion!=streamCollision;
    if(!published && !residencyChanged && requestCursor>=renderOrder.size())return;
    auto previousCollision=std::move(streamCollision);streamCollision=collisionRegion;
    streamPassEye=eye;streamDirty=false;
    int lowA=center[0]-2,highA=center[0]+2;
    if(eye[0]>=8)lowA=std::max(lowA,0);
    if(renderType==2){lowA=int(std::floor((eye[0]-11.)/32.));highA=int(std::floor((eye[0]+11.)/32.));}
    bool readyOrder=planned && ready.lowA==lowA && ready.highA==highA;
    std::vector<Position> addedView,removedView;
    if(planned || verticalChanged) {
        std::set<Position> nextView;
        if(readyOrder)nextView=std::move(ready.view);
        else for(auto column:viewColumns)for(int a=lowA;a<=highA;++a) {column[0]=a;nextView.insert(column);}
        if(readyOrder && viewSnapshot && ready.baseRevision==viewRevision) {
            addedView=std::move(ready.added);removedView=std::move(ready.removed);
        } else {
            std::set_difference(nextView.begin(),nextView.end(),viewWanted.begin(),viewWanted.end(),std::back_inserter(addedView));
            std::set_difference(viewWanted.begin(),viewWanted.end(),nextView.begin(),nextView.end(),std::back_inserter(removedView));
        }
        auto previousSnapshot=std::move(viewSnapshot);
        viewSnapshot=readyOrder?std::move(ready.snapshot):nullptr;
        if(!addedView.empty() || !removedView.empty()) {
            ++viewRevision;
            auto retired=std::move(viewWanted);
            viewWanted=std::move(nextView);lookupDirty=true;
            BackgroundWork::rendering().addTaskFuture([retired=std::move(retired),snapshot=std::move(previousSnapshot)] {});
            mesh.changeColumns(addedView,removedView);
            for(const auto& key:addedView)pendingUploads.insert(key);
            for(const auto& key:removedView) {
                mesh.invalidate(key);rendered.erase(key);renderedMasks.erase(key);pendingUploads.erase(key);
            }
        }
    }
    bool wantedChanged=false;
    std::vector<Position> addedWanted,removedWanted;
    if(residencyChanged && !fullResidency) {
        auto remove=[&](const Position& key) {
            if(!viewWanted.contains(key) && !std::binary_search(collisionRegion.begin(),collisionRegion.end(),key) && wanted.erase(key))
                removedWanted.push_back(key);
        };
        for(const auto& key:removedView)remove(key);
        for(const auto& key:previousCollision)
            remove(key);
        for(const auto& key:addedView)if(wanted.insert(key).second)addedWanted.push_back(key);
        for(const auto& key:collisionRegion)if(wanted.insert(key).second)addedWanted.push_back(key);
        wantedChanged=!addedWanted.empty() || !removedWanted.empty();
    } else if(residencyChanged) {
    auto previousWanted=std::move(wanted);
    wanted=viewWanted;
    wanted.insert(collisionRegion.begin(),collisionRegion.end());
    // Off-slice circuits use sparse logical records, not terrain residency.
    for(const auto& [p,r]:regions) {
        auto lo=structures::translated(p,r.offset),hi=lo;
        for(int i=0;i<N;++i)hi[i]+=r.size[i]-1;
        each(address(lo).chunk,address(hi).chunk,[&](Position key){wanted.insert(key);});
        wanted.insert(address(p).chunk);
    }
    wantedChanged=wanted!=previousWanted;
    if(wantedChanged)std::set_difference(wanted.begin(),wanted.end(),previousWanted.begin(),previousWanted.end(),std::back_inserter(addedWanted));
    if(wantedChanged)std::set_difference(previousWanted.begin(),previousWanted.end(),wanted.begin(),wanted.end(),std::back_inserter(removedWanted));
    }
    for(const auto& key:collisionRegion)space->request(key);
    auto registerColumn=[&](const Position& key) {if(space->find(key)&&registered.insert(key).second) {
        if(temporal)temporal->chunkLoaded(key);else {logic->chunkLoaded(key);falling.loaded(*space,key);}
        if(!temporal)if(auto chunk=space->find(key))
            for(const auto& [a,id]:chunk->extendedBlocks())if(id==ExtendedBlock::WATER){auto p=key;p[0]=key[0]*32+int32_t(a&31);water.changed(p,id);}
    }};
    for(const auto& key:addedWanted)registerColumn(key);
    for(const auto& key:newColumns)if(wanted.contains(key))registerColumn(key);
    bool removed=false;
    if(wantedChanged){
        for(const auto& key:removedWanted)if(registered.erase(key)) {
            mesh.invalidate(key);rendered.erase(key);renderedMasks.erase(key);lookupDirty=true;removed=true;
        }
    }
    if(removed) {logic->retainChunks(wanted);if(temporal)temporal->retainChunks(wanted);else falling.retainChunks(wanted);}
    for(const auto& key:newColumns)if(!wanted.contains(key))removedWanted.push_back(key);
    if(!removedWanted.empty())space->release(removedWanted);
    if(fullResidency && reorder && wantedChanged) {
    if(readyOrder) {
        renderOrder=std::move(ready.order);
        if(!regions.empty())for(const auto& key:wanted)if(!viewWanted.contains(key))renderOrder.push_back(key);
    } else {
    std::vector<std::pair<double,Position>> ranked;ranked.reserve(wanted.size());
    auto distance=[&](const Position& p){double sum=0;for(int i=0;i<N;++i){double delta=(double(p[i])+.5)*extent[i]-eye[i];sum+=delta*delta;}return sum;};
    for(const auto& key:wanted)ranked.emplace_back(distance(key),key);
    std::sort(ranked.begin(),ranked.end());
    renderOrder.clear();renderOrder.reserve(ranked.size());
    for(const auto& [distance,key]:ranked)renderOrder.push_back(key);
    }
    requestCursor=0;
    } else if(!addedWanted.empty()) {
        // Keep unfinished requests, then append only the new band nearest-first.
        renderOrder.erase(renderOrder.begin(),renderOrder.begin()+requestCursor);requestCursor=0;
        auto distance=[&](const Position& key) {
            double sum=0;for(int axis=0;axis<N;++axis){double d=(double(key[axis])+.5)*extent[axis]-eye[axis];sum+=d*d;}return sum;
        };
        std::vector<std::pair<double,Position>> ranked;ranked.reserve(addedWanted.size());
        for(const auto& key:addedWanted)ranked.emplace_back(distance(key),key);
        std::sort(ranked.begin(),ranked.end());
        for(const auto& [distance,key]:ranked)renderOrder.push_back(key);
    }
    unsigned requests=0;
    while(requestCursor<renderOrder.size() && space->pendingCount()<256 && requests<256) {
        const auto& key=renderOrder[requestCursor++];
        if(!wanted.contains(key) || space->find(key))continue;
        space->request(key);++requests;
    }
    space->flush();
}

void uploadMasks(const std::vector<int>& source)
{
    std::vector<int> projected;
    for(size_t base=0;base<source.size();base+=faceStride) {
        bool inside=true;
        for(int axis=1;axis<N;++axis)if(!rayGroup.active[axis] && source[base+axis]!=rayGroup.slice[axis]){inside=false;break;}
        if(!inside)continue;
        for(int axis:rayGroup.axes)projected.push_back(source[base+axis]);
        uint32_t folded=uint32_t(source[base+N]);
        for(size_t word=1;word<FaceMask::wordCount;++word)folded^=TextureAxisWindow<N>::hash(uint32_t(source[base+N+word]));
        projected.push_back(std::bit_cast<int>(folded));
        projected.insert(projected.end(),source.begin()+base+N,source.begin()+base+N+FaceMask::wordCount);
    }
    const size_t stride=rayGroup.axes.size()+1+FaceMask::wordCount;
    maskCount=int(projected.size()/stride);
    if(projected.empty())projected.resize(stride);
    masks.fit(projected.size()*4);masks.uploadData(0,projected.size()*4,projected.data());
}

void upload()
{
    if(renderType==2)mesh.clear();else mesh.configure(eye,frame.axes[0],frame.axes[1]);
    auto changed=space->takeRenderChanges();pendingUploads.merge(changed);
    if(meshViewDirty)for(const auto& key:viewWanted)if(!rendered.contains(key))pendingUploads.insert(key);
    if(meshViewDirty || mesh.needsSynchronization()) {mesh.synchronize(viewWanted);meshViewDirty=false;}
    int budget=256;
    while(!pendingUploads.empty() && budget>0) {
        auto key=*pendingUploads.begin();pendingUploads.erase(pendingUploads.begin());
        if(!viewWanted.contains(key)) {
            if(rendered.erase(key)) {mesh.invalidate(key);lookupDirty=true;}
            renderedMasks.erase(key);continue;
        }
        auto chunk=space->find(key);if(!chunk)continue;
        --budget;
        rendered[key].blocks=chunk->blocks;
        std::vector<int> faceMasks;
        for(const auto& [a,id]:chunk->extendedBlocks())if(ExtendedBlock::isRedirector(id)) {
            auto p=position(key,{a,0u,0u,0u,0u});
            faceMasks.insert(faceMasks.end(),p.begin(),p.end());
            for(auto word:logic->state(p).faces.words)faceMasks.push_back(std::bit_cast<int>(word));
            faceMasks.push_back(0);
        }
        if(faceMasks.empty())renderedMasks.erase(key);else renderedMasks[key]=std::move(faceMasks);
        mesh.invalidate(key);
        const_cast<ColumnChunk*>(chunk)->shouldUpdateRenderer[0]=false;
        lookupDirty=true;
    }
    if(renderType!=2)mesh.update(rendered,eye);
    if(!lookupDirty)return;
    // The mesh shader needs face metadata, but never reads the raycaster's voxel table.
    if(renderType!=2 && mesh.enabled()) {
        std::vector<int> faceMasks;
        for(const auto& [key,data]:renderedMasks)if(viewWanted.contains(key))faceMasks.insert(faceMasks.end(),data.begin(),data.end());
        uploadMasks(faceMasks);
        lookupDirty=false;return;
    }
    auto directAxes=SliceColumns::alignedAxes(streamFrame.axes[0],streamFrame.axes[1]);
    bool direct=renderType!=2 && directAxes[0]>=0 && directAxes[1]>=0 && !viewWanted.empty();
    Position lower{},upper{};
    if(direct) {
        lower=upper=*viewWanted.begin();
        for(const auto& key:viewWanted)for(int i=0;i<N;++i) {
            lower[i]=std::min(lower[i],key[i]);upper[i]=std::max(upper[i],key[i]);
        }
    }
    int origin[3]{},size[3]{1,1,1};
    if(direct)for(int i=0;i<3;++i) {
        int axis=i==0?0:directAxes[i-1];
        origin[i]=lower[axis];size[i]=upper[axis]-lower[axis]+1;
    }
    size_t capacity=1;while(capacity<viewWanted.size()*2+1)capacity*=2;
    size_t stride=rayGroup.axes.size()+1;
    std::vector<uint32_t> table(direct?size_t(size[0])*size[1]*size[2]:capacity*stride,0),data;
    data.reserve(rendered.size()*32);
    std::vector<int> faceMasks;
    for(const auto& [key,gpu]:rendered) {
        if(!viewWanted.contains(key) || !rayGroup.contains(key))continue;
        if(direct) {
            size_t slot=(key[0]-origin[0])+size[0]*((key[directAxes[0]]-origin[1])+size[1]*(key[directAxes[1]]-origin[2]));
            table[slot]=uint32_t(data.size()/32+1);
        } else {
            size_t slot=rayGroup.hash(key)&(capacity-1);
            while(table[slot*stride+stride-1])slot=(slot+1)&(capacity-1);
            for(size_t i=0;i<rayGroup.axes.size();++i)table[slot*stride+i]=uint32_t(key[rayGroup.axes[i]]);
            table[slot*stride+stride-1]=uint32_t(data.size()/32+1);
        }
        data.insert(data.end(),gpu.blocks.begin(),gpu.blocks.end());
        if(auto chunk=space->find(key))for(const auto& [a,id]:chunk->extendedBlocks())if(ExtendedBlock::isRedirector(id)) {
            auto p=position(key,{a,0u,0u,0u,0u});
            faceMasks.insert(faceMasks.end(),p.begin(),p.end());
            for(auto word:logic->state(p).faces.words)faceMasks.push_back(std::bit_cast<int>(word));
            faceMasks.push_back(0);
        }
    }
    if(data.empty())data.resize(32);
    handles.fit(data.size()*4);handles.uploadData(0,data.size()*4,data.data());
    lookup.fit(table.size()*4);lookup.uploadData(0,table.size()*4,table.data());
    shader->use();shader->setUniform("lookupMask",int(capacity-1));
    shader->setUniform("directLookup",direct);
    shader->setUniform("lookupAxes",2,directAxes.data());
    shader->setUniform("lookupOrigin",3,origin);shader->setUniform("lookupSize",3,size);
    frameData.lookupSlice=lower;
    uploadMasks(faceMasks);
    lookupDirty=false;
}

bool intersects(const Vector& p,bool* pending=nullptr)
{
    if(pending)*pending=false;
    bool blocked=false;
    for(auto q:(renderType==2?SideViewND::columns(p,frame.axes[0],.25):SliceColumns::enumerate(p,frame.axes[0],frame.axes[1],.25)))
        for(int a=(int)std::floor(p[0]-1.65);a<=(int)std::floor(p[0]+.1);++a) {
            q[0]=a;auto id=space->loadedBlock(q);
            if(!id){space->request(address(q).chunk);blocked=true;if(pending)*pending=true;}else blocked|=ExtendedBlock::isSolid(*id);
        }
    return blocked;
}

void move(double dt)
{
    if(keys.contains(GLFW_KEY_TAB) || (renderType==2 && middleMouse)) return;
    Vector motion{};
    double speed=playerSpeed*(keys.contains(GLFW_KEY_LEFT_CONTROL)?2.:1.);
    int fb=int(keys.contains(GLFW_KEY_W))-int(keys.contains(GLFW_KEY_S));
    int lr=int(keys.contains(GLFW_KEY_D))-int(keys.contains(GLFW_KEY_A));
    auto heading=frame.heading(yaw), lateral=frame.right(yaw);
    for(int i=1;i<N;++i)motion[i]=fb*heading[i]+lr*lateral[i];
    int pairs[7][2]={{GLFW_KEY_Q,GLFW_KEY_E},{GLFW_KEY_R,GLFW_KEY_F},{GLFW_KEY_T,GLFW_KEY_G},{GLFW_KEY_Y,GLFW_KEY_H},{GLFW_KEY_I,GLFW_KEY_K},{GLFW_KEY_O,GLFW_KEY_L},{GLFW_KEY_U,GLFW_KEY_J}};int hidden=0;
    for(;hidden<7;++hidden) {
        int direction=int(keys.contains(pairs[hidden][1]))-int(keys.contains(pairs[hidden][0]));
        int axisSlot=bankHidden(hidden);
        if(axisSlot>=0 && axisSlot<N-1)for(int i=1;i<N;++i)motion[i]+=direction*frame.axes[axisSlot][i];
    }
    if(renderType==2) {
        motion={};
        for(int pair=0;pair<9;++pair) {
            int slot=pair==0?0:bankHidden(pair-1);
            if(slot>=0 && slot<N-1)for(int axis=1;axis<N;++axis)
                motion[axis]+=(int(sideControls[pair*2+1])-int(sideControls[pair*2]))*frame.axes[slot][axis];
        }
    }
    int selectedDirection=int(keys.contains(GLFW_KEY_PAGE_UP))-int(keys.contains(GLFW_KEY_PAGE_DOWN));
    if(selectedHidden>=0 && selectedHidden<N-1)for(int axis=1;axis<N;++axis)motion[axis]+=selectedDirection*frame.axes[selectedHidden][axis];
    if(flight) {motion[0]=int(keys.contains(GLFW_KEY_SPACE))-int(keys.contains(GLFW_KEY_LEFT_SHIFT));verticalSpeed=0;}
    else {if(grounded && keys.contains(GLFW_KEY_SPACE)) verticalSpeed=10.;verticalSpeed-=32.*dt;}
    double length=0;for(double n:motion) length+=n*n;
    if(length>1) for(double& n:motion) n/=std::sqrt(length);
    for(double& n:motion) n*=speed*dt;
    if(!flight) motion[0]=verticalSpeed*dt;
    grounded=false;
    for(int axis=0;axis<N;++axis)
    {
        if(motion[axis]==0.)continue;
        int steps=std::max(1,int(std::ceil(std::abs(motion[axis])/.2)));
        for(int step=0;step<steps;++step)
        {
            auto next=eye;next[axis]+=motion[axis]/steps;
            if(!noclip && intersects(next)) {if(axis==0) {grounded=motion[0]<0;verticalSpeed=0;}break;}
            eye=next;
        }
    }
}

void set(const Position& p, BlockState value)
{
    if(temporal)temporal->edit(p,value);else logic->set(p,value);
    lookupDirty=true;streamDirty=true;
    falling.changed(*space,p);
    if(!temporal)water.changed(p,value.id);
    if(value.id!=ExtendedBlock::STRUCTURE) regions.erase(p);
}

void fall()
{
    auto moves=falling.moves(*space);
    for(const auto& [from,to]:moves)
        if(space->loadedBlock(to)==Block::AIR) {auto id=space->loadedBlock(from);if(id) {set(to,{*id});set(from,{Block::AIR});}}
}

void update(StateManager& s,double dt)
{
    if(configuringAxes)refreshAxisGroupKeys();
    try
    {
        if(editing) updateRegion();
        structureWork();
        auto previous=eye;
        if(!menu && !editing && !inventory && !console.open) move(std::clamp(dt,0.,.05));
        if(renderType==2 && dt>0 && !menu && !editing && !inventory && !console.open) {
            double distance=0;for(int i=1;i<N;++i)distance+=(eye[i]-previous[i])*frame.axes[0][i];
            int direction=std::abs(distance)>.01*dt?(distance>0?1:-1):0;
            sideWalkTime=direction==0 || direction!=sideWalkDirection?0:std::fmod(sideWalkTime+dt,.32);
            sideWalkDirection=direction;
        }
        camera();
        if(!menu) {
            if(temporal) {if(temporal->advance(dt))lookupDirty=true;}
            else {logic->advance(std::clamp(dt,0.,.25));fallTimer+=dt;if(fallTimer>=.5) {fallTimer=0;fall();}
                water.advance(dt,[this](const Position& p){return space->simulationBlock(p);},
                    [this](const Position&,const Position& to){space->restore(to,ExtendedBlock::WATER);falling.changed(*space,to);lookupDirty=true;},
                    [](const Position&){return 1.;});}
        }
        if(renderType==2)sideTarget(s);else target=space->trace(eye,forward,8.);
        std::string text=std::format("FPS: {:.1f}  Max: {:.1f} ms\n{}D",frameMeter.fps,frameMeter.peakMs,N);
        int shown=0;
        auto displayed=axisGroups.groups[controlBank];
        for(int axis:{0,plane[0],plane[1]})if(std::find(displayed.begin(),displayed.end(),axis)==displayed.end())displayed.push_back(axis);
        for(int axis:displayed) {
            text+=std::format("{}{} {:.1f}",shown++%4==0?"\n":"  ",WorldCommands::axisName(axis),eye[axis]);
        }
        int selectedAxis=1;if(selectedHidden>=0)for(int axis=2;axis<N;++axis)
            if(std::abs(frame.axes[selectedHidden][axis])>std::abs(frame.axes[selectedHidden][selectedAxis]))selectedAxis=axis;
        text+=std::format("\nControls {}/{}  Hidden {} ({})  Columns {}  Pending {}{}{}",controlBank+1,axisGroups.groups.size(),selectedHidden,selectedHidden<0?"-":WorldCommands::axisName(selectedAxis),space->loadedCount(),space->pendingCount(),flight?"  Fly":"",noclip?"  Noclip":"");
        if(ExtendedBlock::isLever(target.block))text+=target.block==ExtendedBlock::LEVER_ON?"\nLever: ON":"\nLever: OFF";
        else if(ExtendedBlock::isCable(target.block))text+=std::format("\nWire: {} / 15",ExtendedBlock::cablePower(target.block));
        coords.setText(text);
        if(temporal)coords.setText(coords.getText()+std::format("\nTime t1/t2: {}/{}",temporal->percent(),100-temporal->percent()));
        selection.setText(blockName(selected));
        blockIcon(selectedIcon,selected,64);
    }
    catch(const std::exception& e) {status.setText(e.what());if(!menu && !editing && !inventory) openPause(s);}
}

void sideTarget(StateManager& s)
{
    target={};sidePlacement.reset();
    int w,h;s.getSize(&w,&h);
    if(w<=0 || h<=0)return;
    auto point=SideViewND::point(eye,frame.axes[0],mouseX,mouseY,w,h);
    double distance=0;for(int i=0;i<N;++i)distance+=(point[i]-eye[i])*(point[i]-eye[i]);
    if(distance>144.)return;
    auto p=cell(point);auto id=space->loadedBlock(p);if(!id)return;
    target.position=p;target.block=*id;target.side=SideViewND::side(point,frame.axes[0]);
    sidePlacement=*id==Block::AIR?std::optional<Position>{p}:neighbor(p,target.side);
}

void render(StateManager& s)
{
    frameMeter.sample(glfwGetTime());
    const auto begin=std::chrono::steady_clock::now();
    if(rayGroup.update(axisGroups,controlBank,plane,eye))lookupDirty=true;
    try {stream();} catch(const std::exception& e) {status.setText(e.what());return;}
    const auto streamed=std::chrono::steady_clock::now();
    upload();
    streamMs=std::chrono::duration<double,std::milli>(streamed-begin).count();
    uploadMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-streamed).count();
    int w,h;s.getSize(&w,&h);
    if(w<=0 || h<=0)return;
    if(!worldResolution.begin(w,h)){worldResolution.percent=100;status.setText("World resolution unavailable; using 100%");}
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glDisable(GL_DEPTH_TEST);
    auto program=renderType!=2 && mesh.enabled()?meshShader:shader;
    program->use(); handles.use(1);textures.use(2);tiles.use(3);masks.use(4);
    const auto profileAxes=textureProfileAxes(axisGroups,controlBank,frame.axes[0],frame.axes[1]);
    program->setUniform("profileAxes",5,profileAxes.data());
    program->setUniform("rayAxisCount",int(rayGroup.axes.size()));
    program->setUniform("rayAxes",int(rayGroup.axes.size()),rayGroup.axes.data());
    textureWindow.update(axisGroups,controlBank,eye,frame.axes[0],frame.axes[1]);
    program->setUniform("textureAxisCount",int(textureWindow.axes.size()));
    std::copy(textureWindow.axes.begin(),textureWindow.axes.end(),frameData.textureWindowAxes.begin());
    program->setUniform("textureCacheEnabled",program==meshShader);
    if(program==meshShader){
        frameData.textureCachedCell=textureWindow.cellHashes;
        frameData.textureCachedSub=textureWindow.subcells;
    }
    for(int i=0;i<N;++i){frameData.eye[i]=float(eye[i]);frameData.forward[i]=float(forward[i]);frameData.right[i]=float(right[i]);frameData.up[i]=float(up[i]);}
    if(program==shader)program->setUniform("sideView",renderType==2);
    if(renderType==2) {
        auto half=SideViewND::halfSize(w,h);program->setUniform("sideHalfSize",glm::vec2{float(half[0]),float(half[1])});
        int sideAxes[2]{1,1};
        for(int axis=2;axis<N;++axis) {
            if(std::abs(right[axis])>std::abs(right[sideAxes[0]]))sideAxes[0]=axis;
            if(std::abs(forward[axis])>std::abs(forward[sideAxes[1]]))sideAxes[1]=axis;
        }
        program->setUniform("sideAxes",2,sideAxes);
        double phase=0;for(int hidden=1;hidden<(N-1);++hidden)for(int axis=1;axis<N;++axis)phase+=eye[axis]*frame.axes[hidden][axis]*(1.+hidden*.137);
        program->setUniform("sidePhase",float(phase));
        const char* name=!flight&&!grounded?"jump":sideWalkDirection==0?"idle":sideWalkTime<.16?"walk1":sideWalkDirection<0?"walk2-left":"walk2-right";
        auto sprite=Texture::get(std::string("player2d.")+name);
        program->setUniform("playerSprite",sprite->getHandle());
        program->setUniform("playerSize",glm::vec2{.875f,1.75f});
    }
    int textureAxes[N-5];std::fill(std::begin(textureAxes),std::end(textureAxes),-1);
    // Two visible directions need planar UVs; remaining axes select depth sections.
    for(int pick=0;pick<2;++pick) {
        int extra=-1;double best=.001;
        for(int candidate=0;candidate<N-5;++candidate) {
            double weight=std::abs(frame.axes[0][5+candidate])+std::abs(frame.axes[1][5+candidate]);
            if(textureAxes[candidate]<0 && weight>best){best=weight;extra=candidate;}
        }
        if(extra<0)break;
        int mapped=-1;double least=3.;
        for(int axis=4;axis>=1;--axis) {
            if(std::find(std::begin(textureAxes),std::end(textureAxes),axis)!=std::end(textureAxes))continue;
            double weight=std::abs(frame.axes[0][axis])+std::abs(frame.axes[1][axis]);
            if(weight<least){least=weight;mapped=axis;}
        }
        textureAxes[extra]=mapped;
    }
    std::copy(std::begin(textureAxes),std::end(textureAxes),frameData.textureExtraAxes.begin());
    program->setUniform("aspect",float(w)/std::max(h,1));lookup.use(5);program->setUniform("maskCount",maskCount);
    program->setUniform("redirector",int(ExtendedBlock::REDIRECTOR_0));
    program->setUniform("doorOpen",TextureProfiles::find(ExtendedBlock::CRYSTAL_DOOR_OPEN)?-1:int(ExtendedBlock::CRYSTAL_DOOR_OPEN));
    frameData.target=target.position;program->setUniform("targetSide",target.block?target.side:-1);
    program->setUniform("targetInSlice",target.block && rayGroup.contains(target.position));
    bool preview=regions.contains(owner);program->setUniform("preview",preview);
    if(preview) {auto lo=structures::translated(owner,regions.at(owner).offset);for(int i=0;i<N;++i){frameData.boxMin[i]=float(lo[i]);frameData.boxMax[i]=float(double(lo[i])+regions.at(owner).size[i]);}}
    bool previewInSlice=preview;int previewFixedBoundaries=0;
    if(preview)for(int axis=0;axis<N;++axis)if(!rayGroup.active[axis]) {
        float point=float(eye[axis]),lo=frameData.boxMin[axis],hi=frameData.boxMax[axis],margin=renderType==2?.04f:0.f;
        previewInSlice &= point>=lo-margin && point<=hi+margin;
        previewFixedBoundaries += std::min(std::abs(point-lo),std::abs(point-hi))<.045f;
    }
    program->setUniform("previewInSlice",previewInSlice);
    program->setUniform("previewFixedBoundaries",previewFixedBoundaries);
    static_assert(sizeof(frameData)==4*(12*size_t(N)-5));
    frameBuffer.fit(sizeof(frameData));frameBuffer.uploadData(0,sizeof(frameData),&frameData);frameBuffer.use(7);
    if(renderType!=2 && mesh.enabled()) {
        mesh.draw(*program,eye,forward,right,up,float(w)/std::max(h,1));
        if(preview) {
            auto low=structures::translated(owner,regions.at(owner).offset),high=low;
            for(int i=0;i<N;++i)high[i]+=regions.at(owner).size[i];
            mesh.drawBox(*program,eye,low,high);
        }
    } else QuadRendererBasic::render();
    worldResolution.finish();
    glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    hud.render();
    console.render(w,h);
    if(menu || editing || inventory) {panel.setMode(QuadRenderer::MODE_FILL);panel.setPos(w/2-(editing?410:240),h/2-285,editing?820:480,570);panel.setColor({.025f,.04f,.035f,.96f});panel.render();}
}

void windowResize(StateManager&,int w,int h)
{
    coords.setWrapWidth(std::max(1,w-24));
    selection.setWrapWidth(std::max(1,w-40));
    hud.windowResize(w,h);
}

void cursor(StateManager& s,bool visible)
{
    keys.clear();sideControls={};bankKeyHeld=false;firstMouse=true;middleMouse=false;
    glfwSetInputMode(s.getWindow(),GLFW_CURSOR,visible || renderType==2?GLFW_CURSOR_NORMAL:GLFW_CURSOR_DISABLED);
    if(renderType==2){glfwGetCursorPos(s.getWindow(),&mouseX,&mouseY);firstMouse=false;}
    if(!visible) s.setUiPage(nullptr);
}

void mouseInput(StateManager&,double x,double y)
{
    if(console.open){mouseX=x;mouseY=y;return;}
    if(firstMouse) {mouseX=x;mouseY=y;firstMouse=false;return;}
    if(!menu && !inventory && !editing && !keys.contains(GLFW_KEY_TAB)) {
        double dx=(x-mouseX)*.004,dy=-(y-mouseY)*.004;
        if(renderType==2) {
            if(middleMouse) {
                int hidden=keys.contains(GLFW_KEY_LEFT_CONTROL)&&keys.contains(GLFW_KEY_LEFT_ALT)&&keys.contains(GLFW_KEY_LEFT_SHIFT)?7:keys.contains(GLFW_KEY_LEFT_CONTROL)&&keys.contains(GLFW_KEY_LEFT_ALT)?6:keys.contains(GLFW_KEY_LEFT_CONTROL)?(keys.contains(GLFW_KEY_LEFT_SHIFT)?5:std::min(4,5)):
                    keys.contains(GLFW_KEY_LEFT_ALT)?std::min(3,5):keys.contains(GLFW_KEY_LEFT_SHIFT)?2:1;
                hidden=(!keys.contains(GLFW_KEY_LEFT_CONTROL)&&!keys.contains(GLFW_KEY_LEFT_ALT)&&!keys.contains(GLFW_KEY_LEFT_SHIFT))?selectedHidden:bankHidden(hidden-1);
                if(hidden<0 || hidden>=(N-1)){mouseX=x;mouseY=y;return;}
                SideViewND::rotate<N>(frame.axes,hidden,dx);freePlane=true;
            }
        } else if(middleMouse) {
            bool ctrl=keys.contains(GLFW_KEY_LEFT_CONTROL)||keys.contains(GLFW_KEY_RIGHT_CONTROL);
            bool alt=keys.contains(GLFW_KEY_LEFT_ALT)||keys.contains(GLFW_KEY_RIGHT_ALT);
            int hidden=keys.contains(GLFW_KEY_LEFT_SHIFT)&&!ctrl&&!alt?7:ctrl && alt && keys.contains(GLFW_KEY_LEFT_SHIFT)?6:ctrl && alt?5:ctrl?4:alt?3:2;
            hidden=(!ctrl&&!alt&&!keys.contains(GLFW_KEY_LEFT_SHIFT))?selectedHidden:bankHidden(hidden-2);
            if(hidden<0 || hidden>=(N-1)){mouseX=x;mouseY=y;return;}
            frame.turnHidden(yaw,hidden,dx,dy);freePlane=true;
        } else {yaw+=dx;pitch=std::clamp(pitch+dy,-1.55,1.55);}
        camera();
    }
    mouseX=x;mouseY=y;
}

void mouseButtonInput(StateManager& s,int button,int action,int)
{
    if(console.open)return;
    if(button==GLFW_MOUSE_BUTTON_MIDDLE) {
        middleMouse=action==GLFW_PRESS && !menu && !editing && !inventory;return;
    }
    if(action!=GLFW_PRESS || menu || editing || inventory) return;
    if(renderType==2)sideTarget(s);
    if(!target.block && !(renderType==2 && sidePlacement && button==GLFW_MOUSE_BUTTON_RIGHT))return;
    if(button==GLFW_MOUSE_BUTTON_LEFT)
    {
        if(ExtendedBlock::isCrystalDoor(target.block))
            for(int side:{1,0}) if(auto n=neighbor(target.position,side);n)
                if(auto id=space->loadedBlock(*n);id && ExtendedBlock::isCrystalDoor(*id)) {set(*n,{Block::AIR});break;}
        set(target.position,{Block::AIR});
    }
    if(button==GLFW_MOUSE_BUTTON_RIGHT)
    {
        if(target.block==ExtendedBlock::STRUCTURE) {openStructure(s,target.position);return;}
        if(temporal?temporal->interact(target.position,target.side):logic->interact(target.position,target.side)) {lookupDirty=true;return;}
        auto p=renderType==2?sidePlacement:neighbor(target.position,target.side);if(!p || space->loadedBlock(*p)!=Block::AIR) return;
        auto raised=eye;
        if(renderType==2){if(SideViewND::overlaps(eye,*p,frame.axes[0]))raised[0]=double((*p)[0])+2.6501;}
        else raised=abovePlacedBlock(eye,*p,frame);
        if(raised!=eye && intersects(raised)) return;
        auto overlaps=[&](const Position& q){return renderType==2?SideViewND::overlaps(raised,q,frame.axes[0]):bodyOverlaps(raised,q,frame);};
        if(selected==ExtendedBlock::CRYSTAL_DOOR)
        {
            std::optional<Position> pair;
            for(int side:{1,0}) if(auto n=neighbor(*p,side);n && space->loadedBlock(*n)==Block::AIR && !overlaps(*n)) {pair=n;break;}
            if(!pair) return;set(*pair,{selected});
        }
        set(*p,{selected});
        if(raised!=eye) {eye=raised;verticalSpeed=0;grounded=true;}
        if(selected==ExtendedBlock::STRUCTURE) regions.try_emplace(*p);
    }
}

void keyInput(StateManager& s,int key,int scan,int action,int mods)
{
    if(console.open) {
        if(action==GLFW_PRESS && (key==GLFW_KEY_ESCAPE || key==GLFW_KEY_GRAVE_ACCENT)) {
            console.open=false;cursor(s,false);
        } else console.key(key,action,mods);
        return;
    }
    if(key==GLFW_KEY_GRAVE_ACCENT && action==GLFW_PRESS && !menu && !editing && !inventory) {
        cursor(s,true);s.setUiPage(nullptr);console.show();return;
    }
    const char* labelName=glfwGetKeyName(key,scan);
    // The character callback handles layout and modifiers; ignore OS key repeats.
    bankKeyHeld=action==GLFW_REPEAT;
    char label=labelName && labelName[0] && !labelName[1]?(char)std::tolower((unsigned char)labelName[0]):'\0';
    int control=SideViewND::control(label,18);
    if(action==GLFW_RELEASE && control>=0)sideControls[control]=false;
    if(action==GLFW_RELEASE) {keys.erase(key);if(key==GLFW_KEY_TAB) firstAxis=-1;return;}
    if(action!=GLFW_PRESS && action!=GLFW_REPEAT) return;
    if(key==GLFW_KEY_ESCAPE)
    {
        if(configuringAxes){configuringAxes=false;openPause(s);return;}
        if(menu || inventory || editing) {menu=inventory=editing=false;cursor(s,false);}else openPause(s);
        return;
    }
    if(menu || editing || inventory) return;
    if(temporal && (key==GLFW_KEY_KP_1 || key==GLFW_KEY_KP_2)) {
        if(action==GLFW_PRESS) {
            temporal->changeSector(key==GLFW_KEY_KP_1?1:-1);
            if(!noclip) {
                auto candidate=eye;
                for(int lift=0;lift<=16;++lift,candidate[0]+=.25) {
                    bool pending=false;bool occupied=intersects(candidate,&pending);
                    if(pending)break;
                    if(!occupied){eye=candidate;grounded=false;break;}
                }
            }
            lookupDirty=true;meshViewDirty=true;streamDirty=true;
            verticalSpeed=0;
        }
        return;
    }
    if((key==GLFW_KEY_HOME || key==GLFW_KEY_END) && action==GLFW_PRESS) {
        cycleHidden(key==GLFW_KEY_END?1:-1);return;
    }
    keys.insert(key);
    if(keys.contains(GLFW_KEY_TAB))
    {
        const char* name=glfwGetKeyName(key,scan);int axis=-1;
        if(name && name[0] && !name[1]) {
            int slot=std::tolower((unsigned char)name[0])-'a';
            if(slot>=0 && slot<int(axisGroups.groups[controlBank].size()))axis=axisGroups.groups[controlBank][slot];
        }
        if(axis>0 && axis<N && action==GLFW_PRESS)
        {
            if(renderType==2) {
                plane={axis,axis==1?2:1};SideViewND::align<N>(frame.axes,axis);
                HyperplaneAlignment::centerHorizontal(eye,N);target={};sidePlacement.reset();streamDirty=true;meshViewDirty=true;
                selectedHidden=bankHidden(0);sideControls={};freePlane=false;yaw=pitch=0;camera();streamValid=false;return;
            }
            if(firstAxis<0) firstAxis=axis;
            else if(axis!=firstAxis) {plane={firstAxis,axis};frame.align(plane);selectedHidden=bankHidden(0);HyperplaneAlignment::centerHorizontal(eye,N);target={};sidePlacement.reset();streamValid=false;streamDirty=true;meshViewDirty=true;freePlane=false;yaw=pitch=0;camera();}
        }
        return;
    }
    if(renderType==2 && control>=0 && !(key==GLFW_KEY_S && (mods&GLFW_MOD_CONTROL) && (mods&GLFW_MOD_SHIFT))) {
        sideControls[control]=true;return;
    }
    if(action==GLFW_PRESS && key==GLFW_KEY_V) {flight=!flight;verticalSpeed=0;}
    if(action==GLFW_PRESS && key==GLFW_KEY_C) noclip=!noclip;
    if(action==GLFW_PRESS && key==GLFW_KEY_S && (mods&GLFW_MOD_CONTROL) && (mods&GLFW_MOD_SHIFT))
        try {save();} catch(const std::exception& e) {openPause(s);status.setText(e.what());}
}

void charInput(StateManager& s,uint32_t code)
{
    if(console.open){console.character(code);return;}
    if(int step=AxisGroups::shortcutStep(code);step && !menu && !editing && !inventory && !keys.contains(GLFW_KEY_TAB)) {
        if(!bankKeyHeld){switchControlBank(step);bankKeyHeld=true;}return;
    }
    if(code=='!' && !menu && !editing) {if(inventory) {inventory=false;cursor(s,false);}else openInventory(s);}
}
void scrollInput(StateManager&,double,double y)
{
    if(console.open)return;
    if(menu || editing || inventory || y==0) return;
    if(middleMouse) {cycleHidden(y>0?1:-1);return;}
    auto at=std::find(selectable.begin(),selectable.end(),selected);int index=at==selectable.end()?0:int(at-selectable.begin());
    for(size_t n=0;n<selectable.size();++n) {index=(index+(y>0?1:-1)+(int)selectable.size())%(int)selectable.size();if(!excluded.contains(selectable[index])) {selected=selectable[index];break;}}
}

void placeElement(ui::element& e,int x,int y)
{
    e.setAlignX(ui::ALIGN_CENTER_X);e.setAlignY(ui::ALIGN_CENTER_Y);e.setOffsetX(x);e.setOffsetY(y);page.addElem(&e);
}
void openPause(StateManager& s)
{
    configuringAxes=false;console.open=false;menu=true;inventory=editing=false;cursor(s,true);page.clear();title.setText(("Paused - "+dimName()));placeElement(title,0,-235);
    resolutionLabel.setText("World resolution");resolutionLabel.setSize(1);placeElement(resolutionLabel,0,-180);
    for(int i=0;i<3;++i){
        int percent=100-i*25;auto& choice=resolutionChoices[i];
        choice.setText(std::to_string(percent)+"%");choice.setChecked(worldResolution.percent==percent);
        choice.setAction([this,&s,percent](bool){worldResolution.percent=percent;openPause(s);});
        placeElement(choice,(i-1)*125,-130);
    }
    resumeButton.setText("Resume");resumeButton.setSize(220,42);resumeButton.setAction([this,&s]{menu=false;cursor(s,false);});placeElement(resumeButton,0,-55);
    saveButton.setText("Save");saveButton.setSize(220,42);saveButton.setAction([this]{try{save();status.setText("Saved");}catch(const std::exception& e){status.setText(e.what());}});placeElement(saveButton,0,5);
    reloadButton.setText("Reload Structures");reloadButton.setSize(290,42);reloadButton.setAction([this]{try {reloadStructures();status.setText("Generation rules updated");}catch(const std::exception& e){status.setText(e.what());}});placeElement(reloadButton,0,65);
    configAxesButton.setText("Config Axis groups");configAxesButton.setSize(290,42);configAxesButton.setAction([this,&s]{openAxisGroups(s);});placeElement(configAxesButton,0,125);
    quitButton.setText("Save and Quit");quitButton.setSize(220,42);quitButton.setAction([this,&s]{try {save();s.changeState(StateTitleScreen::instance());}catch(const std::exception& e){status.setText(e.what());}});placeElement(quitButton,0,185);
    status.setSize(1);status.setWrapWidth(430);placeElement(status,0,245);s.setUiPage(&page);
}
void openInventory(StateManager& s)
{
    inventory=true;cursor(s,true);page.clear();title.setText(("Inventory - "+dimName()));placeElement(title,0,-235);
    int pages=((int)selectable.size()+7)/8;inventoryPage=(inventoryPage+pages)%pages;
    for(int i=0;i<8;++i)
    {
        int index=inventoryPage*8+i;if(index>=(int)selectable.size()) break;auto id=selectable[index];
        checks[i].setText(blockName(id));checks[i].setChecked(!excluded.contains(id));
        checks[i].setAction([this,id,i](bool enabled){
            if(enabled) excluded.erase(id);
            else if(excluded.size()+1<selectable.size()) excluded.insert(id);
            else checks[i].setChecked(true);
            if(excluded.contains(selected)) for(auto next:selectable) if(!excluded.contains(next)) {selected=next;break;}
        });
        blockIcon(inventoryIcons[i],id,28);
        placeElement(inventoryIcons[i],-185,-180+i*42);
        placeElement(checks[i],25,-180+i*42);
    }
    previousButton.setText("<");nextButton.setText(">");previousButton.setSize(50,36);nextButton.setSize(50,36);
    previousButton.setAction([this,&s]{--inventoryPage;openInventory(s);});nextButton.setAction([this,&s]{++inventoryPage;openInventory(s);});
    placeElement(previousButton,-90,205);placeElement(nextButton,90,205);
    resumeButton.setText("Close");resumeButton.setSize(100,36);resumeButton.setAction([this,&s]{inventory=false;cursor(s,false);});placeElement(resumeButton,0,250);
    s.setUiPage(&page);
}

void openAxisGroups(StateManager& s)
{
    configuringAxes=true;axisConfigPage=0;status.setText("");
    for(size_t g=0;g<axisGroups.groups.size();++g)for(size_t slot=0;slot<axisGroups.groups[g].size();++slot){
        int axis=axisGroups.groups[g][slot];
        groupInputs[axis].setText(std::to_string(g+1));orderInputs[axis].setText(std::to_string(slot+1));
        groupInputs[axis].setSize(70,28);orderInputs[axis].setSize(70,28);
    }
    layoutAxisGroups(s);
}
void layoutAxisGroups(StateManager& s)
{
    page.clear();title.setText("Config Axis groups");placeElement(title,0,-245);
    groupColumn.setText("Group");orderColumn.setText("Order");keyColumn.setText("TAB +");
    for(auto t:{&groupColumn,&orderColumn,&keyColumn})t->setSize(1);
    placeElement(groupColumn,-55,-195);placeElement(orderColumn,40,-195);placeElement(keyColumn,145,-195);
    for(int row=0;row<8;++row){
        int axis=axisConfigPage*8+row;if(axis>=N)break;int y=-160+row*36;
        groupAxisLabels[row].setText(WorldCommands::axisName(axis));groupAxisLabels[row].setSize(2);
        placeElement(groupAxisLabels[row],-175,y+5);
        placeElement(groupInputs[axis],-55,y);placeElement(orderInputs[axis],40,y);
        groupKeyLabels[row].setSize(1);placeElement(groupKeyLabels[row],145,y+8);
    }
    axesPrevious.setText("<");axesNext.setText(">");axesPrevious.setSize(40,30);axesNext.setSize(40,30);
    axesPrevious.setAction([this,&s]{axisConfigPage=(axisConfigPage+(N+7)/8-1)%((N+7)/8);layoutAxisGroups(s);});
    axesNext.setAction([this,&s]{axisConfigPage=(axisConfigPage+1)%((N+7)/8);layoutAxisGroups(s);});
    axesPageLabel.setText(std::format("{} / {}",axisConfigPage+1,(N+7)/8));axesPageLabel.setSize(1);
    placeElement(axesPrevious,-80,145);placeElement(axesNext,80,145);placeElement(axesPageLabel,0,154);
    applyAxesButton.setText("Apply");applyAxesButton.setSize(100,36);
    applyAxesButton.setAction([this,&s]{try{
        std::map<int,std::map<int,int>> slots;
        auto integer=[](const std::string& text,int max){
            if(text.empty() || text.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Group and order must be positive integers.");
            int n=std::stoi(text);if(n<1 || n>max)throw std::runtime_error("Invalid group or order (order: 1-9).");return n;
        };
        for(int axis=0;axis<N;++axis){
            int group=integer(groupInputs[axis].getText(),N),slot=integer(orderInputs[axis].getText(),9);
            if(!slots[group].emplace(slot,axis).second)throw std::runtime_error("Two axes occupy the same group/order.");
        }
        std::vector<std::vector<int>> value;
        for(const auto& [g,rows]:slots){value.emplace_back();for(const auto& [slot,axis]:rows)value.back().push_back(axis);}
        axisGroups.assign(std::move(value),N);controlBank=0;activateControlBank();save();status.setText("Axis groups saved");openPause(s);
    }catch(const std::exception& e){status.setText(e.what());}});
    resetAxesButton.setText("Reset");resetAxesButton.setSize(100,36);resetAxesButton.setAction([this,&s]{
        AxisGroups defaults(N);
        for(size_t g=0;g<defaults.groups.size();++g)for(size_t slot=0;slot<defaults.groups[g].size();++slot){int axis=defaults.groups[g][slot];groupInputs[axis].setText(std::to_string(g+1));orderInputs[axis].setText(std::to_string(slot+1));}
        status.setText("");layoutAxisGroups(s);
    });
    resumeButton.setText("Back");resumeButton.setSize(100,36);resumeButton.setAction([this,&s]{openPause(s);});
    placeElement(applyAxesButton,-120,205);placeElement(resetAxesButton,0,205);placeElement(resumeButton,120,205);
    status.setSize(1);status.setWrapWidth(430);placeElement(status,0,255);
    refreshAxisGroupKeys();s.setUiPage(&page);
}
void refreshAxisGroupKeys()
{
    for(int row=0;row<8 && axisConfigPage*8+row<N;++row){
        int axis=axisConfigPage*8+row;auto text=orderInputs[axis].getText();
        if(text.size()!=1 || text[0]<'1' || text[0]>'9'){groupKeyLabels[row].setText("-");continue;}
        int rank=0;
        for(int other=0;other<N;++other)if(groupInputs[other].getText()==groupInputs[axis].getText()){
            auto order=orderInputs[other].getText();if(order.size()==1 && order[0]>='1' && order[0]<text[0])++rank;
        }
        groupKeyLabels[row].setText(rank<9?std::string(1,char('A'+rank)):"-");
    }
}

void layoutStructureFields()
{
    for(int i=0;i<N;++i){page.removeElem(&axes[i]);page.removeElem(&offsets[i]);page.removeElem(&lengths[i]);}
    int first=structurePage*16,last=std::min(N,first+16);
    for(int i=first;i<last;++i){
        int slot=i-first,column=slot/8,row=slot%8,center=column?200:-200;
        placeElement(axes[i],center-160,-150+row*35);
        placeElement(offsets[i],center-55,-150+row*35);
        placeElement(lengths[i],center+80,-150+row*35);
    }
    axesPageLabel.setText(std::format("{} / {}",structurePage+1,(N+15)/16));
}

void openStructure(StateManager& s,Position p)
{
    owner=p;auto& r=regions.try_emplace(p).first->second;
    editing=true;cursor(s,true);page.clear();title.setText(("Structure Block - "+dimName()));placeElement(title,0,-250);
    nameInput.setText(r.name);nameInput.setSize(290,32);placeElement(nameInput,0,-205);
    offsetLabel.setText("Offset / Size");sizeLabel.setText("Offset / Size");offsetLabel.setSize(1);sizeLabel.setSize(1);
    placeElement(offsetLabel,-200,-177);placeElement(sizeLabel,200,-177);
    for(int i=0;i<N;++i)
    {
        axes[i].setText(WorldCommands::axisName(i));axes[i].setSize(1);
        offsets[i].setText(std::to_string(r.offset[i]));offsets[i].setSize(110,30);
        lengths[i].setText(std::to_string(r.size[i]));lengths[i].setSize(110,30);
    }
    structurePage=0;layoutStructureFields();
    if constexpr(N>16) {
        axesPrevious.setText("<");axesNext.setText(">");axesPrevious.setSize(32,28);axesNext.setSize(32,28);
        axesPrevious.setAction([this]{structurePage=(structurePage+(N+15)/16-1)%((N+15)/16);layoutStructureFields();});
        axesNext.setAction([this]{structurePage=(structurePage+1)%((N+15)/16);layoutStructureFields();});
        axesPageLabel.setSize(1);placeElement(axesPrevious,-70,140);placeElement(axesNext,70,140);placeElement(axesPageLabel,0,146);
    }
    previousButton.setText("<");nextButton.setText(">");previousButton.setSize(30,32);nextButton.setSize(30,32);
    previousButton.setAction([this]{chooseStructure(-1);});nextButton.setAction([this]{chooseStructure(1);});
    placeElement(previousButton,-180,-205);placeElement(nextButton,180,-205);
    saveButton.setText("Save");loadButton.setText("Load");resumeButton.setText("Close");
    saveButton.setSize(115,40);loadButton.setSize(115,40);resumeButton.setSize(115,40);
    saveButton.setAction([this]{structureAction(true);});loadButton.setAction([this]{structureAction(false);});
    resumeButton.setAction([this,&s]{updateRegion();editing=false;cursor(s,false);});
    placeElement(saveButton,-135,220);placeElement(loadButton,0,220);placeElement(resumeButton,135,220);
    status.setText("");status.setSize(1);status.setWrapWidth(430);placeElement(status,0,175);
    s.setUiPage(&page);
}

void updateRegion()
{
    if(structureJob!=StructureJob::None) return;
    Region r=regions.at(owner);r.name=nameInput.getText();
    try
    {
        for(int i=0;i<N;++i)
        {
            size_t used=0;r.offset[i]=std::stoi(offsets[i].getText(),&used);
            if(used!=offsets[i].getText().size()) return;
            r.size[i]=std::stoi(lengths[i].getText(),&used);
            if(used!=lengths[i].getText().size() || r.size[i]<1) return;
        }
        auto min=structures::translated(owner,r.offset),last=r.size;for(auto& n:last)--n;structures::translated(min,last);
        regions[owner]=std::move(r);streamDirty=true;
    }
    catch(const std::exception&) {}
}

void chooseStructure(int delta)
{
    if(structureJob!=StructureJob::None) return;
    try
    {
        std::vector<std::string> names;
        for(const auto& entry:std::filesystem::directory_iterator(structures::library("Structure")))
            if(entry.path().extension()==".bin") names.push_back(entry.path().stem().string());
        std::sort(names.begin(),names.end());if(names.empty()) {status.setText("No saved ND structures");return;}
        auto it=std::find(names.begin(),names.end(),nameInput.getText());
        int i=it==names.end()?(delta>0?-1:0):int(it-names.begin());
        nameInput.setText(names[(i+delta+(int)names.size())%names.size()]);
    }
    catch(const std::exception& e) {status.setText(e.what());}
}

void structureAction(bool saving)
{
    if(structureJob!=StructureJob::None) return;
    try
    {
        updateRegion();auto& r=regions.at(owner);
        for(int i=0;i<N;++i)
            if(std::stoi(offsets[i].getText())!=r.offset[i] || std::stoi(lengths[i].getText())!=r.size[i])
                throw std::runtime_error("Invalid region offset or size");
        if(r.name.empty() || r.name=="." || r.name==".." || r.name.find_first_of("<>:\"/\\|?*")!=std::string::npos)
            throw std::runtime_error("Invalid structure name");
        if(!saving)
        {
            std::ifstream in(structures::library("Structure")/(r.name+".bin"),std::ios::binary);
            placing=structures::read(in);r.size=placing->size;
            for(int i=0;i<N;++i) lengths[i].setText(std::to_string(r.size[i]));
        }
        jobOwner=owner;structureJob=saving?StructureJob::Save:StructureJob::Load;status.setText("Loading selected region...");
    }
    catch(const std::exception& e) {status.setText(e.what());}
}

void structureWork()
{
    if(structureJob==StructureJob::None) return;
    try
    {
        auto r=regions.at(jobOwner);auto low=structures::translated(jobOwner,r.offset),high=low;
        for(int i=0;i<N;++i) high[i]+=r.size[i]-1;
        bool ready=true;each(address(low).chunk,address(high).chunk,[&](Position p){ready &= space->find(p)!=nullptr;});
        if(!ready) return;
        if(structureJob==StructureJob::Save)
        {
            auto shape=structures::capture(r.name,low,r.size,*logic);
            auto root=structures::library("Structure"),file=root/(r.name+".bin"),temp=file;temp+=".tmp";
            {std::ofstream out(temp,std::ios::binary);structures::write(out,shape);out.flush();if(!out) throw std::runtime_error("Cannot save structure");}
            replace(temp,file);
            nlohmann::json config{{"structures",nlohmann::json::array()}};
            if(std::filesystem::exists(root/"generation.json")) std::ifstream(root/"generation.json")>>config;
            bool exists=false;for(const auto& entry:config["structures"]) if(entry.value("file","")==file.filename().string()) exists=true;
            if(!exists) config["structures"].push_back({{"file",file.filename().string()},{"frequency",.25},{"spacing",32},
                {"height",{0,127}},{"biome",{"default","test"}},{"enabled",true}});
            jsonFile(root/"generation.json",config);
            reloadStructures();
            status.setText(("Saved to Structure/"+dimName()));
        }
        else
        {
            for(const auto& b:placing->blocks) set(structures::translated(low,b.position),b.state);
            status.setText("Structure loaded");placing.reset();
        }
        structureJob=StructureJob::None;
    }
    catch(const std::exception& e) {structureJob=StructureJob::None;placing.reset();status.setText(e.what());}
}


std::string command(std::string text)
{
    return WorldCommands::execute<N>(std::move(text),{
        eye,playerSpeed,renderType,
        [this](const Position& p,uint16_t id) {
            set(p,{id});
            if(id==ExtendedBlock::STRUCTURE)regions.try_emplace(p);
        },
        [this](const Vector& p) {
            eye=p;verticalSpeed=0;grounded=false;
            streamValid=false;streamDirty=true;meshViewDirty=true;target={};sidePlacement.reset();camera();
        },
        [this](int a,int b) {
            rayGroup.update(axisGroups,controlBank,plane,eye);
            if(!rayGroup.active[a] || (renderType!=2 && !rayGroup.active[b]))
                throw std::runtime_error("Choose axes from the current group, or change Config Axis groups.");
            plane={a,b};
            if(renderType==2)SideViewND::align<N>(frame.axes,a);else frame.align(plane);
            selectedHidden=bankHidden(0);
            HyperplaneAlignment::centerHorizontal(eye,int(N));target={};sidePlacement.reset();
            freePlane=false;yaw=pitch=0;keys.clear();sideControls={};firstAxis=-1;
            camera();streamValid=false;streamDirty=true;meshViewDirty=true;
        },
        [this]{save();}
    });
}

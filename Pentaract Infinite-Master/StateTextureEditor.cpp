#include "StateTextureEditor.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <format>

StateTextureEditor* StateTextureEditor::instance(){static StateTextureEditor e;return &e;}
void StateTextureEditor::updateGroupStatus(){auto samples=profile().groupShapes(referenceFace(),color,activeRegion(referenceFace()));
    groupStatus->setText(std::format("{} / {}\n{} px",groupIndex<0?"New":std::to_string(groupIndex+1),samples.size(),groupDraft.size()));}
void StateTextureEditor::selectGroup(int step){auto list=profile().groupShapes(referenceFace(),color,activeRegion(referenceFace()));
    if(list.empty())return;groupIndex=groupIndex<0?(step>0?0:int(list.size())-1):(groupIndex+step+int(list.size()))%int(list.size());groupDraft=profile().shapes[list[groupIndex]];groupFace=referenceFace();groupSlice=-1;updateGroupStatus();}
void StateTextureEditor::storeGroup(){if(!commitFields())return;try{auto next=profile();bool any=false;
    for(int f=0;f<10;++f)if(f==groupFace&&faces[f]->getChecked()&&color<int(next.count(f))&&(regionIndex<0||activeRegion(f)>=0)){
        auto shape=groupDraft;shape.face=f;shape.color=color;shape.region=activeRegion(f)+1;next.addShape(shape);any=true;
        if(next.group(f,color,0,activeRegion(f))==0)next.group(f,color,0,activeRegion(f))=100;}
    if(!any)return;undo.insert_or_assign(profileId(),profile());profile()=std::move(next);groupDraft={};groupIndex=-1;changed();refreshFields();
    }catch(const std::exception& e){status->setText(e.what());}}
void StateTextureEditor::deleteGroup(){auto next=profile();bool edited=false;
    for(int f=0;f<10;++f)if(faces[f]->getChecked()){auto list=next.groupShapes(f,color,activeRegion(f));if(groupIndex>=0&&groupIndex<int(list.size())){next.shapes.erase(next.shapes.begin()+list[groupIndex]);edited=true;}}
    if(edited){undo.insert_or_assign(profileId(),profile());profile()=std::move(next);groupDraft={};groupIndex=-1;changed();refreshFields();}}
void StateTextureEditor::moveSection(float nextW,float nextV){if(!commitFields())return;w=std::clamp(nextW,0.f,1.f);v=std::clamp(nextV,0.f,1.f);slice=std::min(15,int(w*16));refreshFields();}
void StateTextureEditor::setSectionCoordinates(){
    try{float values[2];ui::text_input* inputs[]{wCoordinate,vCoordinate};for(int i=0;i<2;++i){auto text=inputs[i]->getText();std::replace(text.begin(),text.end(),',','.');size_t used;values[i]=std::stof(text,&used);if(used!=text.size()||!std::isfinite(values[i])||values[i]<0||values[i]>1)throw std::runtime_error("coordinate");}moveSection(values[0],values[1]);}
    catch(...){status->setText("W / V coordinates: 0 to 1");}}
int StateTextureEditor::referenceFace()const{if(paintedFace>=0&&faces[paintedFace]->getChecked())return paintedFace;for(int f=0;f<10;++f)if(faces[f]->getChecked())return f;return 0;}
int StateTextureEditor::activeRegion(int f){return regionIndex>=0&&profile().region(f,regionIndex,0)?regionIndex:-1;}
void StateTextureEditor::changeRegion(int direction){if(!commitFields())return;
    for(int i=0;i<9;++i){regionIndex=(regionIndex+1+direction+9)%9-1;if(regionIndex<0||profile().region(referenceFace(),regionIndex,0))break;}refreshFields();}
void StateTextureEditor::editRegion(bool adding){if(!commitFields())return;auto old=profile();
    if(adding){int slot=-1;for(int r=0;r<TextureProfiles::regionCount;++r){bool free=true,selectedFace=false;for(int f=0;f<10;++f)if(faces[f]->getChecked()){selectedFace=true;free&=profile().region(f,r,0)==0;}if(free&&selectedFace){slot=r;break;}}
        if(slot<0){status->setText("No free zone slot on selected faces");return;}for(int f=0;f<10;++f)if(faces[f]->getChecked())profile().createRegion(f,slot);regionIndex=slot;
    }else{if(regionIndex<0)return;for(int f=0;f<10;++f)if(faces[f]->getChecked()){profile().region(f,regionIndex,0)=0;std::erase_if(profile().shapes,[&](const auto& s){return s.face==f&&s.region==regionIndex+1;});}regionIndex=-1;}
    undo.insert_or_assign(profileId(),old);changed();refreshFields();}
bool StateTextureEditor::commitRegionBounds(){
    if(regionIndex<0)return true;
    try{int bounds[4];for(int i=0;i<4;++i){size_t used;auto str=boundsFields[i]->getText();bounds[i]=std::stoi(str,&used);if(used!=str.size()||bounds[i]<0||bounds[i]>16)throw std::runtime_error("bounds");}
        if(bounds[0]>=bounds[2]||bounds[1]>=bounds[3])throw std::runtime_error("bounds");auto old=profile();
        for(int f=0;f<10;++f)if(faces[f]->getChecked()&&activeRegion(f)>=0)for(int i=0;i<4;++i)profile().region(f,regionIndex,i+1)=bounds[i];
        if(!(old==profile())){undo.insert_or_assign(profileId(),old);changed();}return true;
    }catch(...){status->setText("Zone bounds: 0 <= min < max <= 16");return false;}}
void StateTextureEditor::editColorCount(bool adding){
    if(!commitFields())return;auto old=profile();int first=referenceFace(),next=color;
    for(int f=0;f<10;++f)if(faces[f]->getChecked()){
        if(adding){int index=int(profile().count(f));if(profile().addColor(f)&&f==first)next=index;}
        else profile().removeColor(f,color);}
    if(!(old==profile())){undo.insert_or_assign(profileId(),old);color=next;changed();refreshFields();}
    else status->setText(adding?"Maximum: 16 colors per face":"Keep at least one color per face");
}
TextureProfiles::Profile& StateTextureEditor::profile(){
    auto id=profileId();if(!drafts.contains(id)){auto p=TextureProfiles::find(id);auto& draft=drafts.emplace(id,p?*p:TextureProfiles::Profile(id)).first->second;
        if(!p && id==ExtendedBlock::CRYSTAL_DOOR_OPEN)for(int f=0;f<10;++f)for(int c=0;c<TextureProfiles::colors;++c)draft.transparent(f,c)=1;}
    return drafts.at(id);
}
void StateTextureEditor::changed(){TextureProfiles::apply(profileId(),profile());status->setText("Modified (not saved)");}
void StateTextureEditor::chooseBlock(int step){
    if(!commitRegionBounds()||!commitFields())return;selected=(selected+step+(int)blocks.size())%(int)blocks.size();logicalState=0;regionIndex=-1;paintedFace=-1;w=v=.5f;slice=8;
    TextureProfiles::apply(profileId(),profile());refreshFields();status->setText("Preview");
}
void StateTextureEditor::nextState(){
    int count=TextureProfiles::stateCount(blocks[selected]);
    if(count<=1||!commitRegionBounds()||!commitFields())return;
    logicalState=(logicalState+1)%count;regionIndex=-1;paintedFace=-1;painting=false;
    TextureProfiles::apply(profileId(),profile());refreshFields();status->setText("Preview");
}
void StateTextureEditor::refreshFields(){
    auto& p=profile();auto id=profileId();
    bool variants=TextureProfiles::stateCount(blocks[selected])>1;
    stateButton->setOffsetX(variants?12:-500);editorTitle->setOffsetX(variants?128:12);editorTitle->setSize(variants?1:2);
    if(variants)stateButton->setText(ExtendedBlock::isRedirector(id)?(logicalState?"Face ON":"Face OFF"):
        ExtendedBlock::isLever(id)?(logicalState?"ON":"OFF"):
        blocks[selected]==ExtendedBlock::CRYSTAL_DOOR?(logicalState?"Open":"Closed"):std::format("State {}",logicalState));
    caption->setText(std::string(id<Block::COUNT?Block::names[id]:ExtendedBlock::name(id)));
    int face=referenceFace();color=std::min(color,int(p.count(face))-1);
    auto c=p.rgb(face,color);
    for(int i=0;i<3;++i)fields[i]->setText(std::to_string((c>>(i*8))&255));
    int r=activeRegion(face);
    std::array<int,4> context{int(id),face,color,r};if(context!=groupContext){groupContext=context;groupDraft={};groupIndex=-1;}
    if(groupStatus)updateGroupStatus();
    fields[3]->setText(std::format("{:.2f}",p.weight(face,slice,color,r)/100.));
    for(int a=0;a<3;++a){biasFields[a]->setText(std::to_string(p.bias(face,color,a,r)));displayedBias[a]=biasFields[a]->getText();}
    for(int a=0;a<2;++a){groupFields[a]->setText(std::to_string(p.group(face,color,a,r)));displayedGroup[a]=groupFields[a]->getText();}
    displayedTransparency=p.transparent(face,color)!=0;transparencyToggle->setChecked(displayedTransparency);
    wCoordinate->setText(std::format("{:.4f}",w));vCoordinate->setText(std::format("{:.4f}",v));
    regionLabel->setText(regionIndex<0?"Whole face":r<0?"Zone absent":"Zone "+std::to_string(regionIndex+1));
    for(int i=0;i<4;++i)boundsFields[i]->setText(std::to_string(r<0?(i<2?0:16):p.region(face,r,i+1)));
    displayedWeight=fields[3]->getText();
    fields[4]->setText(std::to_string(p.option(face,0)));fields[5]->setText(std::to_string(p.option(face,1)));
    smooth->setChecked(p.option(face,2)!=0);
    for(int i=0;i<6;++i)displayed[i]=fields[i]->getText();
    for(int i=0;i<8;++i){int index=color/8*8+i;colorButtons[i]->setText(index<int(p.count(face))?std::to_string(index+1):"-");}
}
bool StateTextureEditor::commitFields(bool force){
    try{
        std::array<uint32_t,6> value{};
        for(int i=0;i<6;++i){size_t used=0;auto s=fields[i]->getText();std::replace(s.begin(),s.end(),',','.');double n=std::stod(s,&used);if(used!=s.size()||!std::isfinite(n)||n<0||n>(i<3?255.:i==3||i==5?100.:4294967295.)||(i!=3&&n!=std::floor(n)))throw std::runtime_error("range");value[i]=uint32_t(std::round(n*(i==3?100:1)));}
        uint32_t biasValues[3];for(int a=0;a<3;++a){size_t used;auto s=biasFields[a]->getText();int n=std::stoi(s,&used);if(used!=s.size()||n<0||n>100)throw std::runtime_error("bias");biasValues[a]=n;}
        uint32_t groupValues[2];for(int a=0;a<2;++a){size_t used;auto s=groupFields[a]->getText();int n=std::stoi(s,&used);if(used!=s.size()||n<(a==0?0:1)||n>(a==0?100:256))throw std::runtime_error("grouping");groupValues[a]=n;}
        bool needsShapes=groupValues[0]>0&&groupFields[0]->getText()!=displayedGroup[0]&&profile().groupShapes(referenceFace(),color,activeRegion(referenceFace())).size()<2;
        for(int f=0;f<10;++f)if(faces[f]->getChecked()&&(force||groupFields[1]->getText()!=displayedGroup[1]))for(auto index:profile().groupShapes(f,color,activeRegion(f)))
            if(std::abs(profile().shapes[index].size()-int(groupValues[1]))>1){status->setText("Remove existing groups before changing their pixel count");return false;}
        auto old=profile();auto& p=profile();int ref=referenceFace();bool rgbChanged=force&&regionIndex<0;
        for(int i=0;i<3;++i)rgbChanged|=fields[i]->getText()!=displayed[i];
        bool smoothChanged=smooth->getChecked()!=(old.option(ref,2)!=0);
        for(int f=0;f<10;++f)if(faces[f]->getChecked()){
            int r=activeRegion(f);if(regionIndex>=0&&r<0)continue;
            if(color<int(p.count(f))){if(rgbChanged)p.rgb(f,color)=value[0]|(value[1]<<8)|(value[2]<<16);
                if(force||transparencyToggle->getChecked()!=displayedTransparency)p.transparent(f,color)=transparencyToggle->getChecked()?1:0;
                if(force||fields[3]->getText()!=displayedWeight)for(int s=0;s<16;++s)if(allSlices->getChecked()||s==slice)p.percentage(f,s,color,value[3],r);
                for(int a=0;a<3;++a)if(force||biasFields[a]->getText()!=displayedBias[a])p.bias(f,color,a,r)=biasValues[a];
                for(int a=0;a<2;++a)if(force||groupFields[a]->getText()!=displayedGroup[a])p.group(f,color,a,r)=groupValues[a];}
            if(fields[4]->getText()!=displayed[4])p.option(f,0)=value[4];
            if(fields[5]->getText()!=displayed[5])p.option(f,1)=value[5];
            if(smoothChanged)p.option(f,2)=smooth->getChecked()?1:0;}
        displayedWeight=fields[3]->getText();
        if(!(p==old)){undo.insert_or_assign(profileId(),old);changed();}
        if(needsShapes)highlightToggle->setChecked(true);
        refreshFields();
        return true;
    }catch(...){status->setText("Invalid value: RGB 0-255, % 0-100, pixels 1-256");return false;}
}
void StateTextureEditor::buildUi(StateManager& manager){
    page.clear();controls.clear();
    auto label=[&](std::string text,int x,int y){auto t=add<ui::text>(x,y);t->setText(text);t->setSize(1);return t;};
    auto button=[&](std::string text,int x,int y,int w,auto action){auto b=add<ui::button>(x,y);b->setSize(w,28);b->setText(text);b->setAction(std::move(action));return b;};
    auto check=[&](std::string text,int x,int y,bool value){auto c=add<ui::checkbox>(x,y);c->setText(text);c->setChecked(value);return c;};
    editorTitle=label("Texture Editor",12,10);
    stateButton=button("",12,6,108,[this]{nextState();});
    button("Back",240,6,85,[&manager]{manager.popState();});
    button("<",12,48,40,[this]{chooseBlock(-1);});button(">",285,48,40,[this]{chooseBlock(1);});
    caption=label("",65,55);
    button("Save",12,88,95,[this]{if(!commitRegionBounds()||!commitFields())return;try{TextureProfiles::save(profileId(),profile());status->setText("Saved and applied");}catch(const std::exception& e){status->setText(e.what());}});
    button("Reload",116,88,95,[this]{try{auto p=TextureProfiles::read(TextureProfiles::directory()/(std::to_string(profileId())+".txp"));undo.insert_or_assign(profileId(),profile());profile()=p;changed();refreshFields();status->setText("Loaded");}catch(const std::exception& e){status->setText(e.what());}});
    button("Undo",220,88,105,[this]{auto it=undo.find(profileId());if(it!=undo.end()){std::swap(profile(),it->second);changed();refreshFields();}});
    for(int i=0;i<8;++i)colorButtons[i]=button(std::to_string(i+1),12+i*39,154,34,[this,i]{if(!commitFields())return;int next=color/8*8+i;if(next<int(profile().count(referenceFace()))){color=next;refreshFields();}});
    button("+ Color",panel+12,80,120,[this]{editColorCount(true);});
    button("- Color",panel+140,80,120,[this]{editColorCount(false);});
    button("<",panel+268,80,40,[this]{if(commitFields()){color=std::max(0,color-8);refreshFields();}});
    button(">",panel+316,80,40,[this]{if(commitFields()){color=std::min(int(profile().count(referenceFace()))-1,color+8);refreshFields();}});
    const char* names[]={"R","G","B","Proportion %","Seed","V influence %"};
    for(int i=0;i<6;++i){int x=i<3?12+i*105:12,y=i<3?188:i==3?234:i==4?496:538;
        label(names[i],x,y);fields[i]=add<ui::text_input>(i<3?x:x+115,y+12);fields[i]->setSize(i<3?95:195,26);}
    button("Apply",230,246,95,[this]{if(commitRegionBounds())commitFields(true);});fields[3]->setSize(100,26);
    button("-",12,284,40,[this]{moveSection(w-1.f/16,v);});
    button("+",58,284,40,[this]{moveSection(w+1.f/16,v);});
    label("W",104,274);wCoordinate=add<ui::text_input>(104,284);wCoordinate->setSize(100,28);
    allSlices=check("All W",205,284,false);
    smooth=check("Smooth W",12,316,true);erase=check("Erase",205,316,false);
    const char* fn[]={"Y-","Y+","X-","X+","Z-","Z+","W-","W+","V-","V+"};
    for(int f=0;f<10;++f){int x=12+(f%5)*65,y=354+(f/5)*32;faces[f]=check("",x,y,true);label(fn[f],x+29,y+6);faces[f]->setAction([this](bool){refreshFields();});}
    button("All",12,422,72,[this]{for(auto f:faces)f->setChecked(true);refreshFields();});
    button("Sides",92,422,105,[this]{for(int i=0;i<10;++i)faces[i]->setChecked(i>=2);refreshFields();});
    button("Top",205,422,120,[this]{for(int i=0;i<10;++i)faces[i]->setChecked(i==1);refreshFields();});
    button("Copy W",12,458,195,[this]{if(!commitFields())return;undo.insert_or_assign(profileId(),profile());auto& p=profile();for(int f=0;f<10;++f)if(faces[f]->getChecked()&&(regionIndex<0||activeRegion(f)>=0)){int r=activeRegion(f);for(int s=0;s<16;++s)for(int c=0;c<TextureProfiles::colors;++c)p.weight(f,s,c,r)=p.weight(f,slice,c,r);p.authoredSlices(f,r)=0;}changed();});
    planeButton=button("XZ",220,458,105,[this]{constexpr int axes[6][2]={{1,2},{1,3},{1,4},{2,3},{2,4},{3,4}};plane=(plane+1)%6;xAxis=axes[plane][0];zAxis=axes[plane][1];});
    button("-",12,588,40,[this]{moveSection(w,v-1.f/16);});button("+",58,588,40,[this]{moveSection(w,v+1.f/16);});
    label("V",104,578);vCoordinate=add<ui::text_input>(104,588);vCoordinate->setSize(100,28);
    button("Reset",205,588,120,[this]{undo.insert_or_assign(profileId(),profile());TextureProfiles::Profile defaults(profileId());auto& p=profile();for(int f=0;f<10;++f)if(faces[f]->getChecked()){
        std::erase_if(p.shapes,[&](const auto& s){return s.face==f&&(regionIndex<0||s.region==regionIndex+1);});groupDraft={};groupIndex=-1;
        if(regionIndex>=0){if(activeRegion(f)>=0){p.authoredSlices(f,regionIndex)=p.authoredSlices(f);for(int s=0;s<16;++s)for(int c=0;c<TextureProfiles::colors;++c)p.weight(f,s,c,regionIndex)=p.weight(f,s,c);for(int c=0;c<TextureProfiles::colors;++c){for(int a=0;a<3;++a)p.bias(f,c,a,regionIndex)=50;p.group(f,c,0,regionIndex)=0;p.group(f,c,1,regionIndex)=4;}}continue;}
        p.authoredSlices(f)=0;p.count(f)=defaults.count(f);for(int c=0;c<TextureProfiles::colors;++c){p.rgb(f,c)=defaults.rgb(f,c);p.transparent(f,c)=profileId()==ExtendedBlock::CRYSTAL_DOOR_OPEN;for(int a=0;a<3;++a)p.bias(f,c,a)=50;p.group(f,c,0)=0;p.group(f,c,1)=4;}for(int r=0;r<TextureProfiles::regionCount;++r)p.region(f,r,0)=0;
        for(int i=0;i<3;++i)p.option(f,i)=defaults.option(f,i);for(int s=0;s<16;++s){for(int c=0;c<TextureProfiles::colors;++c)p.weight(f,s,c)=defaults.weight(f,s,c);for(int y=0;y<16;++y)for(int x=0;x<16;++x)p.pin(f,s,x,y)=0;}}changed();refreshFields();});
    status=label("Preview",panel+12,12);status->setWrapWidth(std::max(100,width-panel-24));
    distribution=label("",panel+12,45);distribution->setWrapWidth(std::max(100,width-panel-24));
    regionLabel=label("Whole face",panel+12,128);
    button("<",panel+130,120,40,[this]{changeRegion(-1);});button(">",panel+176,120,40,[this]{changeRegion(1);});
    button("+ Zone",panel+222,120,100,[this]{editRegion(true);});button("- Zone",panel+328,120,100,[this]{editRegion(false);});
    const char* boundsNames[]={"U min","V min","U max","V max"};
    for(int i=0;i<4;++i){label(boundsNames[i],panel+12+i*105,158);boundsFields[i]=add<ui::text_input>(panel+12+i*105,170);boundsFields[i]->setSize(95,26);}
    const char* biasNames[]={"X+ %","Y+ %","Z+ %"};
    for(int i=0;i<3;++i){label(biasNames[i],panel+12+i*110,202);biasFields[i]=add<ui::text_input>(panel+12+i*110,214);biasFields[i]->setSize(100,26);}
    button("Set",panel+342,214,86,[this]{if(commitRegionBounds())commitFields(true);});
    label("Groupement %",panel+12,246);label("Pixels / group",panel+180,246);
    for(int i=0;i<2;++i){groupFields[i]=add<ui::text_input>(panel+12+i*168,258);groupFields[i]->setSize(150,26);}
    highlightToggle=check("",panel+342,258,false);label("Highlight",panel+376,266);
    button("Add",panel+12,328,100,[this]{storeGroup();});
    button("New",panel+120,328,60,[this]{groupDraft={};groupIndex=-1;updateGroupStatus();});
    button("<",panel+188,328,32,[this]{selectGroup(-1);});button(">",panel+228,328,32,[this]{selectGroup(1);});
    button("Del",panel+268,328,80,[this]{deleteGroup();});
    groupStatus=label("",panel+358,335);groupStatus->setWrapWidth(std::max(100,width-panel-370));
    transparencyToggle=check("Transparency",panel+12,290,false);
    button("Go",panel+250,290,64,[this]{setSectionCoordinates();});
    button("0.5",panel+324,290,88,[this]{moveSection(.5f,.5f);});
    refreshFields();manager.setUiPage(&page);
}
void StateTextureEditor::init(StateManager& s){
    if(blocks.empty()){for(int id=1;id<Block::COUNT;++id)blocks.push_back(id);for(int id=ExtendedBlock::FIRST;id<ExtendedBlock::COUNT;++id)if(ExtendedBlock::isSelectable(id))blocks.push_back(id);}
    if(!vao){
        if(!shader.initFiles({{GL_VERTEX_SHADER,"assets/shaders/textureEditor.vert"},{GL_FRAGMENT_SHADER,"assets/shaders/textureEditor.frag"}}))throw std::runtime_error("Texture Editor shader failed");
        glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);quad=QuadRenderer(Shader::get("quad"));quad.init();
    }
    w=v=.5f;slice=8;plane=0;xAxis=1;zAxis=2;paintedFace=-1;
    s.getSize(&width,&height);buildUi(s);TextureProfiles::apply(profileId(),profile());orbit=painting=false;
}
void StateTextureEditor::close(StateManager& s){s.setUiPage(nullptr);orbit=painting=false;}
glm::mat4 StateTextureEditor::matrix()const{
    glm::vec3 center(.5f);glm::vec3 eye=center+distance*glm::vec3(std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch));
    return glm::perspective(glm::radians(45.f),float(std::max(1,width-panel))/std::max(1,height-previewTop),.05f,100.f)*glm::lookAt(eye,center,glm::vec3(0,1,0));
}
void StateTextureEditor::render(StateManager&){
    glViewport(0,0,width,height);glClearColor(.035f,.045f,.05f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(panel,0,std::max(1,width-panel),std::max(1,height-previewTop));glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);
    struct Vertex{glm::vec3 p;int face;};std::vector<Vertex> vertices;
    int axes[]={xAxis,0,zAxis};
    bool highlight=highlightToggle->getChecked();
    for(int a=0;a<3;++a)for(int sign=0;sign<2;++sign){int u=(a+1)%3,v=(a+2)%3;glm::vec3 p[4];
        for(int i=0;i<4;++i){p[i]=glm::vec3(0);p[i][a]=float(sign);p[i][u]=(i==1||i==2)?1.f:0.f;p[i][v]=i>=2?1.f:0.f;}
        for(int i:{0,1,2,0,2,3})vertices.push_back({p[i],axes[a]*2+sign});}
    shader.use();shader.setUniform("mvp",matrix());shader.setUniform("blockId",int(profileId()));shader.setUniform("xAxis",xAxis);shader.setUniform("zAxis",zAxis);shader.setUniform("wSlice",w);shader.setUniform("vSlice",v);
    std::array<int,8> highlightMask{};for(int i=0;i<8;++i)highlightMask[i]=std::bit_cast<int>(groupDraft.mask[i]);
    shader.setUniform("highlightFace",highlight?groupFace:-1);shader.setUniform("highlightSlice",groupSlice);shader.setUniform("highlightMask",8,highlightMask.data());
    int faceMask=0;for(int f=0;f<10;++f)if(faces[f]->getChecked())faceMask|=1<<f;shader.setUniform("selectedRegion",regionIndex);shader.setUniform("selectedFaces",faceMask);
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(1);glVertexAttribIPointer(1,1,GL_INT,sizeof(Vertex),(void*)offsetof(Vertex,face));glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glViewport(0,0,width,height);
    quad.setMode(QuadRenderer::MODE_FILL);quad.setPos(0,0,panel,height);quad.setColor(.075f,.085f,.09f);quad.render();
    for(int i=0;i<8;++i){int index=color/8*8+i;auto c=index<int(profile().count(referenceFace()))?profile().rgb(referenceFace(),index):0x222222;quad.setPos(12+i*39,128,34,20);quad.setColor(float(c&255)/255,float((c>>8)&255)/255,float((c>>16)&255)/255);quad.render();
        if(index<int(profile().count(referenceFace()))&&profile().transparent(referenceFace(),index)){for(int y=0;y<2;++y)for(int x=0;x<4;++x){quad.setPos(12+i*39+x*8,128+y*10,x==3?10:8,10);float shade=(x+y)%2?.65f:.25f;quad.setColor(shade,shade,shade);quad.render();}}
        if(index==color){quad.setPos(12+i*39,128,34,20);quad.setMode(QuadRenderer::MODE_LINES);quad.setColor(1.f,1.f,1.f);quad.render();quad.setMode(QuadRenderer::MODE_FILL);}}
    caption->setText(std::format("{} W:{:.3f} V:{:.3f}",profileId()<Block::COUNT?Block::names[profileId()]:ExtendedBlock::name(profileId()),w,v));
    caption->setWrapWidth(215);
    const char* axesNames="YXZWV";planeButton->setText(std::string(1,axesNames[xAxis])+axesNames[zAxis]);
    int f=referenceFace();
    std::string probabilities=std::format("{}{} / W {}: ",axesNames[f/2],f%2?'+':'-',slice);
    for(int c=color/8*8;c<std::min(color/8*8+8,int(profile().count(f)));++c)probabilities+=std::format("{}:{:.1f}% ",c+1,profile().weight(f,slice,c,activeRegion(f))/100.);
    distribution->setText(probabilities);
}
bool StateTextureEditor::paint(){
    if(mx<=panel||mx>=width||my<previewTop||my>=height)return false;
    auto inv=glm::inverse(matrix());float nx=2.f*float(mx-panel)/std::max(1,width-panel)-1,ny=1-2.f*float(my-previewTop)/std::max(1,height-previewTop);
    auto a=inv*glm::vec4(nx,ny,-1,1),b=inv*glm::vec4(nx,ny,1,1);glm::vec3 o=glm::vec3(a)/a.w,d=glm::normalize(glm::vec3(b)/b.w-o);
    float best=1e9f;int face=-1;glm::vec3 hit;int axes[]={xAxis,0,zAxis};
    for(int axis=0;axis<3;++axis)if(std::abs(d[axis])>1e-6)for(int side=0;side<2;++side){float t=(float(side)-o[axis])/d[axis];auto p=o+t*d;if(t<0||t>=best)continue;bool inside=true;for(int j=0;j<3;++j)if(j!=axis&&(p[j]<0||p[j]>1))inside=false;if(inside){best=t;hit=p;face=axes[axis]*2+side;}}
    if(face<0||!faces[face]->getChecked()||color>=int(profile().count(face)))return false;float p[5]={hit.y,.5f,.5f,w,v};p[xAxis]=hit.x;p[zAxis]=hit.z;
    int normal=face/2,u=normal==0?1:0,vAxis=(normal==0||normal==1)?2:1;
    int x=std::clamp(int(p[u]*16),0,15),y=std::clamp(int(p[vAxis]*16),0,15);
    if(regionIndex>=0){if(activeRegion(face)<0||x<int(profile().region(face,regionIndex,1))||y<int(profile().region(face,regionIndex,2))||x>=int(profile().region(face,regionIndex,3))||y>=int(profile().region(face,regionIndex,4)))return false;}
    int actualSlice=std::clamp(int(p[3]*16),0,15);
    if(highlightToggle->getChecked()){
        if(groupDraft.size()==0){paintedFace=face;refreshFields();groupFace=face;groupSlice=actualSlice;}
        if(groupFace!=face||(groupSlice>=0&&groupSlice!=actualSlice))return false;
        bool value=!erase->getChecked(),edited=groupDraft.contains(x,y)!=value;
        if(edited){groupDraft.set(x,y,value);groupDraft.samples[y*16+x]=value?uint8_t(TextureProfiles::sampledColor(profile(),profileId(),face,{p[0],p[1],p[2],p[3],p[4]})+1):0;}
        return edited;
    }
    paintedFace=face;w=std::clamp(p[3],0.f,1.f);v=std::clamp(p[4],0.f,1.f);slice=actualSlice;
    bool edited=false;for(int s=0;s<16;++s)if(allSlices->getChecked()||s==actualSlice){auto& pin=profile().pin(face,s,x,y);uint32_t value=erase->getChecked()?0:color+1;edited|=pin!=value;pin=value;}
    return edited;
}
void StateTextureEditor::mouseInput(StateManager&,double x,double y){
    if(orbit){yaw+=float(x-mx)*.008f;pitch=std::clamp(pitch+float(y-my)*.008f,-1.5f,1.5f);}
    if(painting){double fromX=mx,fromY=my;int steps=std::max(1,int(std::ceil(std::hypot(x-fromX,y-fromY)/2.)));bool edited=false;
        for(int i=1;i<=steps;++i){mx=fromX+(x-fromX)*i/steps;my=fromY+(y-fromY)*i/steps;edited|=paint();}
        if(edited){if(highlightToggle->getChecked())updateGroupStatus();else changed();}}
    mx=x;my=y;
}
void StateTextureEditor::mouseButtonInput(StateManager&,int button,int action,int){
    if(action==GLFW_RELEASE){if(button==GLFW_MOUSE_BUTTON_RIGHT)orbit=false;if(button==GLFW_MOUSE_BUTTON_LEFT){bool painted=painting&&!highlightToggle->getChecked();painting=false;if(painted)refreshFields();}return;}
    if(mx<=panel||my<previewTop)return;if(button==GLFW_MOUSE_BUTTON_RIGHT)orbit=true;
    if(button==GLFW_MOUSE_BUTTON_LEFT){if(!commitFields())return;undo.insert_or_assign(profileId(),profile());painting=true;if(paint()){if(highlightToggle->getChecked())updateGroupStatus();else changed();}}
}
void StateTextureEditor::scrollInput(StateManager&,double,double y){if(mx>panel)distance=std::clamp(distance-float(y)*.2f,1.5f,10.f);}
void StateTextureEditor::windowResize(StateManager&,int w,int h){width=w;height=h;if(status)status->setWrapWidth(std::max(100,w-panel-24));if(distribution)distribution->setWrapWidth(std::max(100,w-panel-24));if(groupStatus)groupStatus->setWrapWidth(std::max(80,w-panel-370));}

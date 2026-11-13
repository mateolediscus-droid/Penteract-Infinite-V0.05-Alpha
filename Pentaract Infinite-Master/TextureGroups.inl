// Included by TextureProfile.cpp; templates are baked only when a profile changes.
namespace TextureProfiles {
void Profile::addShape(GroupShape shape) {
    if(shape.face>=faces||shape.color>=count(shape.face)||shape.region>regionCount)throw std::runtime_error("Invalid group selection");
    int count=shape.size(),target=int(group(shape.face,shape.color,1,int(shape.region)-1));
    if(!count||std::abs(count-target)>1)throw std::runtime_error("Group pixel count must match target (+/- 1)");
    auto remaining=shape;std::vector<std::pair<int,int>> todo;
    for(int y=0;y<16&&todo.empty();++y)for(int x=0;x<16;++x)if(shape.contains(x,y)){todo.emplace_back(x,y);break;}
    int visited=0;
    while(!todo.empty()){auto [x,y]=todo.back();todo.pop_back();if(!remaining.contains(x,y))continue;
        remaining.set(x,y,false);++visited;todo.emplace_back(x-1,y);todo.emplace_back(x+1,y);todo.emplace_back(x,y-1);todo.emplace_back(x,y+1);}
    if(visited!=count)throw std::runtime_error("Each group must be connected");
    for(auto i:groupShapes(shape.face,shape.color,int(shape.region)-1))if(std::abs(shapes[i].size()-count)>1)throw std::runtime_error("Groups must differ by at most one pixel");
    int left=16,top=16;for(int y=0;y<16;++y)for(int x=0;x<16;++x)if(shape.contains(x,y)){left=std::min(left,x);top=std::min(top,y);}
    GroupShape normalized{shape.face,shape.color,shape.region};
    for(int y=top;y<16;++y)for(int x=left;x<16;++x)if(shape.contains(x,y)){normalized.set(x-left,y-top,true);auto sample=shape.samples[y*16+x];if(sample>this->count(shape.face))throw std::runtime_error("Invalid group color");normalized.samples[(y-top)*16+x-left]=sample;}
    shapes.push_back(normalized);
}

std::array<int,256> groupTile(const Profile& source,int face,int slice,int variant) {
    std::array<int,256> tile;tile.fill(-1);
    bool enabled=false;for(int r=-1;r<regionCount;++r)for(int c=0;c<int(source.data[counts+face]);++c)
        enabled|=source.data[groups+((face*(regionCount+1)+r+1)*colors+c)*2]>0&&source.groupShapes(face,c,r).size()>=2;
    if(!enabled)return tile;
    const auto& p=source;
    std::array<int,256> zones;zones.fill(-1);
    for(int i=0;i<256;++i)for(int r=0;r<regionCount;++r)if(p.region(face,r,0)&&i%16>=int(p.region(face,r,1))&&i/16>=int(p.region(face,r,2))&&i%16<int(p.region(face,r,3))&&i/16<int(p.region(face,r,4)))zones[i]=r;
    uint32_t random=hash(p.option(face,0)^uint32_t(face*937+slice*3571+variant*7919));
    auto next=[&](){random=hash(random+0x9e3779b9u);return random;};
    for(int r=-1;r<regionCount;++r) {
        int area=int(std::count(zones.begin(),zones.end(),r));if(!area)continue;
        std::array<std::vector<size_t>,colors> models;
        std::array<double,colors> residual{};
        double background=0;
        for(int c=0;c<int(p.count(face));++c){models[c]=p.groupShapes(face,c,r);
            double strength=models[c].size()>=2?p.group(face,c,0,r)/100.:0.;
            residual[c]=p.weight(face,slice,c,r)*(1.-strength);background+=residual[c];}
        // A fully grouped palette has no background to separate silhouettes.
        if(background==0)for(int c=0;c<int(p.count(face));++c){models[c].clear();residual[c]=p.weight(face,slice,c,r);}
        std::array<int,colors> order;for(int c=0;c<colors;++c)order[c]=c;
        for(int c=colors-1;c>0;--c)std::swap(order[c],order[next()%(c+1)]);
        for(int c:order)if(c<int(p.count(face))&&models[c].size()>=2&&p.group(face,c,0,r)>0) {
            double desired=area*p.weight(face,slice,c,r)/10000.*p.group(face,c,0,r)/100.;int placed=0;
            for(int attempt=0;attempt<1024&&placed<desired;++attempt) {
                const auto& shape=p.shapes[models[c][next()%models[c].size()]];int amount=shape.size();
                if(placed+amount>desired+amount*.5)break;
                int ox=next()%16,oy=next()%16;bool fits=true;
                for(int y=0;y<16&&fits;++y)for(int x=0;x<16;++x)if(shape.contains(x,y)) {
                    int px=x+ox,py=y+oy;if(px>=16||py>=16){fits=false;break;}int i=py*16+px;
                    if(zones[i]!=r||tile[i]>=0||p.pin(face,slice,px,py)){fits=false;break;}
                    for(auto [dx,dy]:{std::pair{-1,0},{1,0},{0,-1},{0,1}}){int nx=px+dx,ny=py+dy;
                        if(nx>=0&&ny>=0&&nx<16&&ny<16&&tile[ny*16+nx]==c)fits=false;}
                }
                if(!fits)continue;
                double acceptance=1;int normal=face/2,u=normal==0?1:0,v=(normal==0||normal==1)?2:1;
                double local[5]{.5,.5,.5,(slice+.5)/16.,.5};local[u]=(ox+.5)/16.;local[v]=(oy+.5)/16.;
                for(int axis=0;axis<3;++axis){int coord=axis==0?1:axis==1?0:2;if(coord==normal)continue;
                    double pref=std::clamp(p.bias(face,c,axis,r)/100.,.01,.99),t=std::clamp((local[coord]-.45)/.1,0.,1.);t=t*t*(3-2*t);
                    acceptance*=((1-pref)*(1-t)+pref*t)/std::max(pref,1-pref);}
                if(next()/4294967296.>acceptance)continue;
                for(int y=0;y<16;++y)for(int x=0;x<16;++x)if(shape.contains(x,y))tile[(oy+y)*16+ox+x]=shape.samples[y*16+x]?int(shape.samples[y*16+x])-1:c;
                placed+=amount;
            }
        }
        for(int i=0;i<256;++i)if(zones[i]==r&&tile[i]<0) {
            double total=0;std::array<double,colors> weights{};int normal=face/2;
            int u=normal==0?1:0,v=(normal==0||normal==1)?2:1;
            double local[5]{.5,.5,.5,(slice+.5)/16.,.5};local[u]=(i%16+.5)/16.;local[v]=(i/16+.5)/16.;
            for(int c=0;c<int(p.count(face));++c){weights[c]=residual[c];
                for(int axis=0;axis<3;++axis){int coord=axis==0?1:axis==1?0:2;if(coord==normal)continue;
                    double pref=std::clamp(p.bias(face,c,axis,r)/100.,.01,.99),t=std::clamp((local[coord]-.45)/.1,0.,1.);t=t*t*(3-2*t);
                    weights[c]*=2*((1-pref)*(1-t)+pref*t);}
                total+=weights[c];}
            double pick=(next()/4294967296.)*total;int chosen=0;
            for(int c=0;c<int(p.count(face));++c){pick-=weights[c];if(pick<0){chosen=c;break;}}tile[i]=chosen;
        }
    }
    return tile;
}
int sampledColor(const Profile& p,uint16_t block,int face,const std::array<float,5>& point){
    std::array<float,5> local;uint32_t h=hash(block^p.option(face,0)),shapeHash=h;
    int normal=face/2,u=normal==0?1:0,v=(normal==0||normal==1)?2:1;
    for(int a=0;a<5;++a){local[a]=std::clamp(point[a],0.f,.99999f);h=hash(h^hash(uint32_t(a)+0x9e3779b9u));h=hash(h^uint32_t(local[a]*16));
        shapeHash=hash(shapeHash);if(a!=u&&a!=v&&a!=normal)shapeHash=hash(shapeHash^uint32_t(local[a]*16));}
    int x=int(local[u]*16),y=int(local[v]*16),painted=int(local[3]*16);auto pin=p.pin(face,painted,x,y);if(pin)return int(pin)-1;
    float w=std::min(local[3]*16,15.f),derived=std::clamp(w+(local[4]-.5f)*15*p.option(face,1)/100.f,0.f,15.f);
    int s0=int(derived),s1=std::min(15,s0+1);float blend=p.option(face,2)?derived-s0:0;
    auto tile=groupTile(p,face,s0,shapeHash%4);if(tile[0]>=0)return tile[y*16+x];
    int region=-1;for(int r=0;r<regionCount;++r)if(p.region(face,r,0)&&x>=int(p.region(face,r,1))&&y>=int(p.region(face,r,2))&&x<int(p.region(face,r,3))&&y<int(p.region(face,r,4)))region=r;
    std::array<float,colors> weights{};float total=0;
    for(int c=0;c<int(p.count(face));++c){weights[c]=p.weight(face,s0,c,region)*(1-blend)+p.weight(face,s1,c,region)*blend;
        for(int a=0;a<3;++a){int coord=a==0?1:a==1?0:2;if(coord==normal)continue;float pref=std::clamp(p.bias(face,c,a,region)/100.f,.01f,.99f),t=std::clamp(((int(local[coord]*16)+.5f)/16-.45f)/.1f,0.f,1.f);t=t*t*(3-2*t);weights[c]*=2*((1-pref)*(1-t)+pref*t);}total+=weights[c];}
    float pick=std::min(float(h)/4294967295.f,.99999994f)*total;for(int c=0;c<int(p.count(face));++c){pick-=weights[c];if(pick<0)return c;}return 0;
}
}

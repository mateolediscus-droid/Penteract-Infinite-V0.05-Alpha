#include "TextureProfile.h"
#include <GL/glew.h>
#include <Windows.h>
#include <cstdio>
#include "TextureGroups.inl"

namespace TextureProfiles {
namespace { std::map<uint16_t,Profile> profiles; bool loaded=false,dirty=true; GLuint buffer=0; }
std::filesystem::path directory() {
    // main selects the installed game's data root before any profile is loaded,
    // including when the executable lives in an ND compilation cache.
    static const auto dataRoot=std::filesystem::current_path();
    return dataRoot/"TextureProfiles";
}
Profile read(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary);uint32_t header[3]{};
    f.read(reinterpret_cast<char*>(header),sizeof(header));
    if(!f||header[0]!=0x31505854)throw std::runtime_error("Invalid texture profile");
    Profile p;
    if(header[1]==1&&header[2]==42252){
        std::vector<uint32_t> old(header[2]);f.read(reinterpret_cast<char*>(old.data()),old.size()*4);
        if(!f)throw std::runtime_error("Truncated texture profile");
        for(int face=0;face<faces;++face){for(int i=0;i<3;++i)p.option(face,i)=old[i];
            for(int c=0;c<8;++c)p.rgb(face,c)=old[4+c];
            for(int s=0;s<slices;++s){uint64_t total=0,cumulative=0;uint32_t allocated=0;
                for(int c=0;c<8;++c)total+=old[12+(face*16+s)*8+c];
                for(int c=0;c<8;++c){cumulative+=old[12+(face*16+s)*8+c];auto next=total?uint32_t(cumulative*10000/total):10000;
                    p.weight(face,s,c)=next-allocated;allocated=next;}
                for(int y=0;y<16;++y)for(int x=0;x<16;++x)p.pin(face,s,x,y)=old[1292+(face*16+s)*256+y*16+x];}}
    }else if(header[1]==2&&header[2]==oldWords)f.read(reinterpret_cast<char*>(p.data.data()),oldWords*sizeof(uint32_t));
    else if(header[1]==3&&header[2]==version3Words)f.read(reinterpret_cast<char*>(p.data.data()),version3Words*sizeof(uint32_t));
    else if(header[1]==4&&header[2]==version4Words)f.read(reinterpret_cast<char*>(p.data.data()),version4Words*sizeof(uint32_t));
    else if(header[1]==5&&header[2]==version5Words)f.read(reinterpret_cast<char*>(p.data.data()),version5Words*sizeof(uint32_t));
    else if((header[1]>=6&&header[1]<=8)&&header[2]==words)f.read(reinterpret_cast<char*>(p.data.data()),p.data.size()*sizeof(uint32_t));
    else throw std::runtime_error("Invalid texture profile version");
    if(header[1]>=7){uint32_t count=0;f.read(reinterpret_cast<char*>(&count),4);if(count>65536)throw std::runtime_error("Too many group shapes");
        for(uint32_t i=0;i<count;++i){GroupShape shape;f.read(reinterpret_cast<char*>(&shape.face),4);f.read(reinterpret_cast<char*>(&shape.color),4);f.read(reinterpret_cast<char*>(&shape.region),4);f.read(reinterpret_cast<char*>(shape.mask.data()),32);if(header[1]>=8)f.read(reinterpret_cast<char*>(shape.samples.data()),256);if(!f)throw std::runtime_error("Truncated group shape");p.addShape(shape);}}
    if(!f||f.peek()!=EOF)throw std::runtime_error("Truncated texture profile");
    if(header[1]<6)for(int face=0;face<faces;++face)for(int r=-1;r<regionCount;++r)p.authoredSlices(face,r)=65535;
    for(int face=0;face<faces;++face){
        if(p.count(face)<1||p.count(face)>colors||p.option(face,1)>100||p.option(face,2)>1)throw std::runtime_error("Invalid texture parameters");
        for(int r=-1;r<regionCount;++r)if(p.authoredSlices(face,r)>65535)throw std::runtime_error("Invalid slice overrides");
        for(int c=0;c<colors;++c)if(p.transparent(face,c)>1)throw std::runtime_error("Invalid transparency");
        for(int r=-1;r<regionCount;++r){if(r>=0){if(p.region(face,r,0)>1)throw std::runtime_error("Invalid region");if(!p.region(face,r,0))continue;
                if(p.region(face,r,1)>=p.region(face,r,3)||p.region(face,r,2)>=p.region(face,r,4)||p.region(face,r,3)>16||p.region(face,r,4)>16)throw std::runtime_error("Invalid region bounds");}
            for(int c=0;c<colors;++c)for(int a=0;a<3;++a)if(p.bias(face,c,a,r)>100)throw std::runtime_error("Invalid XYZ bias");
            for(int c=0;c<colors;++c)if(p.group(face,c,0,r)>100||p.group(face,c,1,r)<1||p.group(face,c,1,r)>256)throw std::runtime_error("Invalid grouping");
            for(int s=0;s<slices;++s){uint32_t sum=0;for(int c=0;c<colors;++c){auto value=p.weight(face,s,c,r);if(value>10000||(c>=p.count(face)&&value))throw std::runtime_error("Invalid region percentage");sum+=value;}if(sum!=10000)throw std::runtime_error("Invalid percentage total");}}
        for(int s=0;s<slices;++s){uint32_t total=0;
            for(int c=0;c<colors;++c){auto value=p.weight(face,s,c);if(value>10000||(c>=p.count(face)&&value))throw std::runtime_error("Invalid texture weight");total+=value;}
            if(total!=10000)throw std::runtime_error("Texture percentages must sum to 100");
            for(int y=0;y<16;++y)for(int x=0;x<16;++x)if(p.pin(face,s,x,y)>p.count(face))throw std::runtime_error("Invalid painted pixel");}}
    return p;
}
static void load() {
    if(loaded)return;loaded=true;
    Profile water(ExtendedBlock::WATER);
    for(int f=0;f<faces;++f)for(int c=0;c<8;++c)
        water.rgb(f,c)=uint32_t(35+c*4)|(uint32_t(100+c*8)<<8)|(uint32_t(185+c*8)<<16);
    profiles.emplace(ExtendedBlock::WATER,std::move(water));
    try {auto dir=directory();if(!std::filesystem::exists(dir))return;
        for(auto& entry:std::filesystem::directory_iterator(dir))if(entry.path().extension()==".txp") {
            try {auto id=std::stoul(entry.path().stem().string());if(id<ExtendedBlock::COUNT){
                if(id==ExtendedBlock::WATER)profiles.insert_or_assign(uint16_t(id),read(entry.path()));
                else profiles.emplace(canonical(uint16_t(id)),read(entry.path()));}}
            catch(const std::exception& e){printf("Texture profile ignored: %s\n",e.what());}
        }
    }catch(const std::exception& e){printf("Texture profiles: %s\n",e.what());}
}
static auto lookup(uint16_t id){
    // A missing powered-state profile must not masquerade as the OFF/zero state.
    // The shader already supplies a deterministic procedural texture per material.
    return profiles.find(canonical(id));
}
const Profile* find(uint16_t id){load();auto it=lookup(id);return it==profiles.end()?nullptr:&it->second;}
void apply(uint16_t id,const Profile& p){load();profiles.insert_or_assign(canonical(id),p);dirty=true;}
void save(uint16_t id,const Profile& p) {
    auto dir=directory();std::filesystem::create_directories(dir);
    auto path=dir/(std::to_string(canonical(id))+".txp"),tmp=path;tmp+=".tmp";
    std::ofstream f(tmp,std::ios::binary|std::ios::trunc);uint32_t header[]={0x31505854,8,words};
    f.write(reinterpret_cast<char*>(header),sizeof(header));f.write(reinterpret_cast<const char*>(p.data.data()),p.data.size()*sizeof(uint32_t));
    uint32_t count=uint32_t(p.shapes.size());f.write(reinterpret_cast<char*>(&count),4);
    for(const auto& s:p.shapes){f.write(reinterpret_cast<const char*>(&s.face),4);f.write(reinterpret_cast<const char*>(&s.color),4);f.write(reinterpret_cast<const char*>(&s.region),4);f.write(reinterpret_cast<const char*>(s.mask.data()),32);f.write(reinterpret_cast<const char*>(s.samples.data()),256);}f.close();
    if(!f)throw std::runtime_error("Cannot write texture profile");
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot commit texture profile");
    apply(id,p);
}
void bind() {
    load();if(!buffer){glGenBuffers(1,&buffer);dirty=true;}
    if(dirty) {
        std::vector<uint32_t> data(ExtendedBlock::COUNT,0);
        for(const auto& [id,p]:profiles){uint32_t offset=(uint32_t)data.size();data.insert(data.end(),p.data.begin(),p.data.end());
            size_t table=data.size();data.resize(table+faces*slices,0);
            for(int face=0;face<faces;++face)for(int slice=0;slice<slices;++slice){auto tile=groupTile(p,face,slice,0);if(tile[0]<0)continue;
                data[table+face*slices+slice]=uint32_t(data.size()-offset);
                for(int variant=0;variant<4;++variant){if(variant)tile=groupTile(p,face,slice,variant);for(int c:tile)data.push_back(uint32_t(c));}}
            data[id]=offset;}
        for(int i=0;i<ExtendedBlock::COUNT;++i)if(auto it=lookup(i);it!=profiles.end())data[i]=data[it->first];
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,buffer);glBufferData(GL_SHADER_STORAGE_BUFFER,data.size()*sizeof(uint32_t),data.data(),GL_DYNAMIC_DRAW);dirty=false;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,6,buffer);
}
void applyOpacity(std::vector<uint8_t>& opacity){
    load();for(size_t id=0;id<opacity.size();++id)if(auto p=find(uint16_t(id))){
        for(int f=0;f<faces;++f)for(uint32_t c=0;c<p->data[counts+f];++c)
            if(p->data[transparency+f*colors+c])opacity[id]=0;
    }
    if(auto on=find(ExtendedBlock::REDIRECTOR_0+1)){
        for(int f=0;f<faces;++f)for(unsigned c=0;c<on->data[counts+f];++c)if(on->data[transparency+f*colors+c]){
            for(size_t id=ExtendedBlock::REDIRECTOR_0;id<opacity.size() && ExtendedBlock::isRedirector(uint16_t(id));++id)opacity[id]=0;
            return;
        }
    }
}
}

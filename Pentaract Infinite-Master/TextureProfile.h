#pragma once
#include "Block.h"
#include <array>
#include <map>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <bit>

namespace TextureProfiles {
constexpr int colors=16, slices=16, faces=10, pixels=256;
constexpr int palette=4, counts=palette+faces*colors, settings=counts+faces;
constexpr int weights=settings+faces*3, pins=weights+faces*slices*colors;
constexpr int oldWords=pins+faces*slices*pixels;
constexpr int biases=oldWords, regions=biases+faces*colors*3, regionCount=8;
constexpr int regionStride=5+slices*colors+colors*3;
constexpr int version3Words=regions+faces*regionCount*regionStride;
constexpr int groups=version3Words, version4Words=groups+faces*(regionCount+1)*colors*2;
constexpr int transparency=version4Words, version5Words=transparency+faces*colors;
constexpr int overrides=version5Words, words=overrides+faces*(regionCount+1);
inline uint32_t hash(uint32_t x) {x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;return x^(x>>16);}
inline uint16_t family(uint16_t id) {
    if(ExtendedBlock::isCable(id))return ExtendedBlock::CABLE_0;
    if(ExtendedBlock::isRepeater(id))return ExtendedBlock::REPEATER_0;
    if(ExtendedBlock::isRedirector(id))return ExtendedBlock::REDIRECTOR_0;
    if(id==ExtendedBlock::LEVER_ON)return ExtendedBlock::LEVER_OFF;
    if(id==ExtendedBlock::CRYSTAL_DOOR_OPEN)return ExtendedBlock::CRYSTAL_DOOR;
    return id;
}
inline uint16_t canonical(uint16_t id) {
    if(ExtendedBlock::isRedirector(id) && id!=ExtendedBlock::REDIRECTOR_0+1)return ExtendedBlock::REDIRECTOR_0;
    return id;
}
inline int stateCount(uint16_t id) {
    if(ExtendedBlock::isCable(id)||ExtendedBlock::isRepeater(id))return 16;
    return ExtendedBlock::isLever(id)||ExtendedBlock::isRedirector(id)||family(id)==ExtendedBlock::CRYSTAL_DOOR?2:1;
}
struct GroupShape {
    uint32_t face=0,color=0,region=0;
    std::array<uint32_t,8> mask{};
    std::array<uint8_t,256> samples{};
    bool contains(int x,int y)const{return x>=0&&y>=0&&x<16&&y<16&&(mask[(y*16+x)/32]&(1u<<((y*16+x)%32)));}
    void set(int x,int y,bool value){auto& word=mask[(y*16+x)/32];auto bit=1u<<((y*16+x)%32);if(value)word|=bit;else word&=~bit;}
    int size()const{int n=0;for(auto word:mask)n+=std::popcount(word);return n;}
    bool operator==(const GroupShape&)const=default;
};
struct Profile {
    std::vector<uint32_t> data=std::vector<uint32_t>(words,0);
    std::vector<GroupShape> shapes;
    explicit Profile(uint32_t seed=1) {
        data[0]=seed;data[1]=50;data[2]=1;
        for(int f=0;f<faces;++f){count(f)=8;option(f,0)=seed;option(f,1)=50;option(f,2)=1;
            for(int i=0;i<colors;++i){rgb(f,i)=hash(seed+i*3571)&0xffffff;for(int a=0;a<3;++a)bias(f,i,a)=50;for(int r=-1;r<regionCount;++r)group(f,i,1,r)=4;}
            for(int s=0;s<slices;++s)for(int c=0;c<8;++c)weight(f,s,c)=1250;}
    }
    uint32_t& rgb(int f,int c){return data[palette+f*colors+c];}
    uint32_t& transparent(int f,int c){return data[transparency+f*colors+c];}
    uint32_t& authoredSlices(int f,int r=-1){return data[overrides+f*(regionCount+1)+r+1];}
    uint32_t& count(int f){return data[counts+f];}
    uint32_t& option(int f,int i){return data[settings+f*3+i];}
    int regionOffset(int f,int r)const{return regions+(f*regionCount+r)*regionStride;}
    uint32_t& region(int f,int r,int field){return data[regionOffset(f,r)+field];}
    uint32_t& weight(int f,int s,int c,int r=-1){return data[r<0?weights+(f*slices+s)*colors+c:regionOffset(f,r)+5+s*colors+c];}
    uint32_t& bias(int f,int c,int a,int r=-1){return data[r<0?biases+(f*colors+c)*3+a:regionOffset(f,r)+5+slices*colors+c*3+a];}
    uint32_t& group(int f,int c,int setting,int r=-1){return data[groups+((f*(regionCount+1)+r+1)*colors+c)*2+setting];}
    std::vector<size_t> groupShapes(int f,int c,int r=-1)const{std::vector<size_t> found;for(size_t i=0;i<shapes.size();++i)if(shapes[i].face==f&&shapes[i].color==c&&shapes[i].region==r+1)found.push_back(i);return found;}
    void addShape(GroupShape shape);
    void createRegion(int f,int r){region(f,r,0)=1;region(f,r,1)=4;region(f,r,2)=4;region(f,r,3)=12;region(f,r,4)=12;authoredSlices(f,r)=authoredSlices(f);
        for(int s=0;s<slices;++s)for(int c=0;c<colors;++c)weight(f,s,c,r)=weight(f,s,c);
        std::erase_if(shapes,[&](const GroupShape& s){return s.face==f&&s.region==r+1;});
        auto original=shapes;for(auto s:original)if(s.face==f&&s.region==0){s.region=r+1;shapes.push_back(s);}
        for(int c=0;c<colors;++c){for(int a=0;a<3;++a)bias(f,c,a,r)=bias(f,c,a);for(int i=0;i<2;++i)group(f,c,i,r)=group(f,c,i);}}
    uint32_t& pin(int f,int s,int x,int y){return data[pins+(f*slices+s)*pixels+y*16+x];}
    uint32_t count(int f)const{return data[counts+f];}
    uint32_t option(int f,int i)const{return data[settings+f*3+i];}
    uint32_t region(int f,int r,int field)const{return data[regionOffset(f,r)+field];}
    uint32_t weight(int f,int s,int c,int r=-1)const{return data[r<0?weights+(f*slices+s)*colors+c:regionOffset(f,r)+5+s*colors+c];}
    uint32_t bias(int f,int c,int a,int r=-1)const{return data[r<0?biases+(f*colors+c)*3+a:regionOffset(f,r)+5+slices*colors+c*3+a];}
    uint32_t group(int f,int c,int setting,int r=-1)const{return data[groups+((f*(regionCount+1)+r+1)*colors+c)*2+setting];}
    uint32_t pin(int f,int s,int x,int y)const{return data[pins+(f*slices+s)*pixels+y*16+x];}
    void rebuildSlices(int f,int r=-1){
        auto anchor=[&](int s){return s==8||(authoredSlices(f,r)&(1u<<s))!=0;};
        for(int s=0;s<slices;++s)if(!anchor(s)){
            int lo=-1,hi=-1;for(int i=s-1;i>=0;--i)if(anchor(i)){lo=i;break;}
            for(int i=s+1;i<slices;++i)if(anchor(i)){hi=i;break;}
            if(lo<0)lo=hi;if(hi<0)hi=lo;
            uint32_t left=0,right=0,allocated=0;
            for(int c=0;c<colors;++c){left+=weight(f,lo,c,r);right+=weight(f,hi,c,r);
                uint32_t next=lo==hi?left:(left*(hi-s)+right*(s-lo))/(hi-lo);
                weight(f,s,c,r)=next-allocated;allocated=next;}
        }
    }
    void percentage(int f,int s,int c,uint32_t value,int r=-1,bool author=true){
        int n=int(count(f));if(c>=n)return;
        if(n==1){weight(f,s,0,r)=10000;return;}
        uint32_t sum=0;for(int i=0;i<n;++i)if(i!=c)sum+=weight(f,s,i,r);
        uint64_t cumulative=0;uint32_t allocated=0;
        for(int i=0;i<n;++i)if(i!=c){cumulative+=sum?weight(f,s,i,r):1;
            uint32_t next=uint32_t(cumulative*(10000-value)/(sum?sum:n-1));weight(f,s,i,r)=next-allocated;allocated=next;}
        weight(f,s,c,r)=value;
        if(author){if(s!=8)authoredSlices(f,r)|=1u<<s;rebuildSlices(f,r);}
    }
    bool addColor(int f){int n=int(count(f));if(n==colors)return false;count(f)++;rgb(f,n)=hash(option(f,0)+n*3571)&0xffffff;transparent(f,n)=0;
        for(int r=-1;r<regionCount;++r)if(r<0||region(f,r,0)){for(int a=0;a<3;++a)bias(f,n,a,r)=50;
            group(f,n,0,r)=0;group(f,n,1,r)=4;
            for(int s=0;s<slices;++s){weight(f,s,n,r)=0;percentage(f,s,n,10000/(n+1),r,false);}}return true;}
    bool removeColor(int f,int c){int n=int(count(f));if(n<=1||c>=n)return false;
        std::erase_if(shapes,[&](const GroupShape& s){return s.face==f&&s.color==c;});
        for(auto& s:shapes)if(s.face==f&&s.color>c)--s.color;
        for(auto& s:shapes)if(s.face==f)for(auto& sample:s.samples){if(sample==c+1)sample=uint8_t(s.color+1);else if(sample>c+1)--sample;}
        for(int r=-1;r<regionCount;++r)if(r<0||region(f,r,0)){
            for(int s=0;s<slices;++s){percentage(f,s,c,0,r,false);for(int i=c;i<n-1;++i)weight(f,s,i,r)=weight(f,s,i+1,r);weight(f,s,n-1,r)=0;}
            for(int i=c;i<n-1;++i){for(int a=0;a<3;++a)bias(f,i,a,r)=bias(f,i+1,a,r);for(int a=0;a<2;++a)group(f,i,a,r)=group(f,i+1,a,r);}}
        for(int s=0;s<slices;++s){
            for(int y=0;y<16;++y)for(int x=0;x<16;++x){auto& p=pin(f,s,x,y);if(p==c+1)p=0;else if(p>c+1)--p;}}
        for(int i=c;i<n-1;++i){rgb(f,i)=rgb(f,i+1);transparent(f,i)=transparent(f,i+1);}--count(f);return true;}
    bool operator==(const Profile&)const=default;
};
// -1 means no authored grouping on this face; otherwise palette indices, including background.
std::array<int,256> groupTile(const Profile&,int face,int slice,int variant);
int sampledColor(const Profile&,uint16_t block,int face,const std::array<float,5>& point);
std::filesystem::path directory();
void save(uint16_t,const Profile&);
Profile read(const std::filesystem::path&);
const Profile* find(uint16_t);
void apply(uint16_t,const Profile&);
void bind();
void applyOpacity(std::vector<uint8_t>&);
}

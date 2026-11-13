#pragma once
#include "TextureProfile.h"
#include "experiments/SliceMesh.h"
#include "experiments/SliceColumns.h"
#include "BackgroundWork.h"
#include "Shader.h"
#include <map>
#include <set>
#include <future>
#include <memory>
#include <cstddef>

template<size_t N> class SliceMeshRenderer {
    friend struct SliceMeshTestAccess;
    using Position=std::array<int32_t,N>;
    using Vector=std::array<double,N>;
    using TileKey=std::array<int,3>;
    struct Tile { GLuint buffer=0;GLsizei count=0; };
    std::map<TileKey,Tile> tiles;
    std::map<TileKey,std::future<std::vector<SliceMesh::Vertex>>> pending;
    std::set<TileKey> wanted,dirty;
    std::map<TileKey,size_t> columnCounts;
    std::array<int,2> axes{-1,-1};
    Position slice{};
    bool active=false;
    bool showOpenDoor=false;
    bool syncNeeded=true;
    GLuint vao=0,lineBuffer=0;
    std::shared_ptr<const std::vector<uint8_t>> opaque=std::make_shared<const std::vector<uint8_t>>();
    static int divide(int n,int d){return n/d-(n%d<0);}
    bool matches(const Position& p) const {
        for(size_t i=1;i<N;++i)if(int(i)!=axes[0] && int(i)!=axes[1] && p[i]!=slice[i])return false;
        return true;
    }
    TileKey tileKey(const Position& p) const {return {p[0],divide(p[axes[0]],SliceMesh::width),divide(p[axes[1]],SliceMesh::width)};}
    void bind(GLuint buffer) {
        if(!vao)glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(SliceMesh::Vertex),(void*)offsetof(SliceMesh::Vertex,position));
        glEnableVertexAttribArray(1);glVertexAttribIPointer(1,3,GL_INT,sizeof(SliceMesh::Vertex),(void*)offsetof(SliceMesh::Vertex,cell));
        glEnableVertexAttribArray(2);glVertexAttribIPointer(2,2,GL_UNSIGNED_INT,sizeof(SliceMesh::Vertex),(void*)offsetof(SliceMesh::Vertex,id));
    }
public:
    void clear() {
        for(auto& [key,tile]:tiles)if(tile.buffer)glDeleteBuffers(1,&tile.buffer);
        tiles.clear();pending.clear();wanted.clear();dirty.clear();columnCounts.clear();
        if(vao)glDeleteVertexArrays(1,&vao);vao=0;
        if(lineBuffer)glDeleteBuffers(1,&lineBuffer);lineBuffer=0;
        active=false;syncNeeded=true;
    }
    void setOpaque(std::vector<uint8_t> value){TextureProfiles::applyOpacity(value);opaque=std::make_shared<const std::vector<uint8_t>>(std::move(value));showOpenDoor=TextureProfiles::find(ExtendedBlock::CRYSTAL_DOOR_OPEN)!=nullptr;}
    bool configure(const Vector& eye,const Vector& x,const Vector& z) {
        auto next=SliceColumns::alignedAxes(x,z);
        if(next[0]<0 || next[1]<0){if(active)clear();return false;}
        Position location{};for(size_t i=1;i<N;++i)location[i]=int(std::floor(eye[i]));
        bool changed=!active || axes!=next;
        for(size_t i=1;i<N;++i)if(int(i)!=next[0] && int(i)!=next[1])changed|=location[i]!=slice[i];
        if(changed){clear();axes=next;slice=location;active=true;}
        return true;
    }
    bool enabled() const{return active;}
    bool needsSynchronization() const{return syncNeeded;}
    size_t faceCount() const {size_t n=0;for(const auto& [key,tile]:tiles)n+=tile.count/6;return n;}
    void invalidate(const Position& p) {
        if(!active || !matches(p))return;
        auto key=tileKey(p);if(wanted.contains(key))dirty.insert(key);
        for(int axis=0;axis<3;++axis)for(int sign:{-1,1}) {
            if(axis>0) {
                int local=p[axes[axis-1]]-key[axis]*SliceMesh::width;
                if(local!=(sign<0?0:SliceMesh::width-1))continue;
            }
            auto other=key;other[axis]+=sign;if(wanted.contains(other))dirty.insert(other);
        }
    }
    void synchronize(const std::set<Position>& columns) {
        if(!active)return;
        syncNeeded=false;
        std::set<TileKey> next;
        columnCounts.clear();
        for(const auto& p:columns)if(matches(p)){auto key=tileKey(p);next.insert(key);++columnCounts[key];}
        for(auto key:next)if(!wanted.contains(key))dirty.insert(key);
        for(auto it=tiles.begin();it!=tiles.end();)if(!next.contains(it->first)){
            if(it->second.buffer)glDeleteBuffers(1,&it->second.buffer);it=tiles.erase(it);
        }else ++it;
        std::erase_if(dirty,[&](auto key){return !next.contains(key);});
        wanted=std::move(next);
    }
    void changeColumns(const std::vector<Position>& added,const std::vector<Position>& removed) {
        if(!active || syncNeeded)return;
        // Add first so a tile shared by outgoing and incoming bands stays resident.
        for(const auto& p:added)if(matches(p)) {
            auto key=tileKey(p);
            if(columnCounts[key]++==0){wanted.insert(key);dirty.insert(key);}
        }
        for(const auto& p:removed)if(matches(p)) {
            auto key=tileKey(p);auto count=columnCounts.find(key);
            if(count==columnCounts.end() || --count->second!=0)continue;
            columnCounts.erase(count);wanted.erase(key);dirty.erase(key);
            if(auto tile=tiles.find(key);tile!=tiles.end()) {
                if(tile->second.buffer)glDeleteBuffers(1,&tile->second.buffer);
                tiles.erase(tile);
            }
        }
    }
    template<class Columns> void update(const Columns& columns,const Vector& eye) {
        if(!active)return;
        int uploadBudget=4;
        for(auto it=pending.begin();it!=pending.end() && uploadBudget>0;) {
            if(it->second.wait_for(std::chrono::seconds(0))!=std::future_status::ready){++it;continue;}
            auto vertices=it->second.get();--uploadBudget;
            // An edit received after submission invalidates this snapshot. Keep
            // the dirty tile queued instead of uploading an obsolete block state.
            if(wanted.contains(it->first) && !dirty.contains(it->first)) {
                auto& tile=tiles[it->first];tile.count=GLsizei(vertices.size());
                if(tile.count) {
                    if(!tile.buffer)glGenBuffers(1,&tile.buffer);
                    glBindBuffer(GL_ARRAY_BUFFER,tile.buffer);
                    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(SliceMesh::Vertex),vertices.data(),GL_DYNAMIC_DRAW);
                }
            }
            it=pending.erase(it);
        }
        auto distance=[&](TileKey key) {
            double a=key[0]*32.+16-eye[0],x=(key[1]+.5)*SliceMesh::width-eye[axes[0]],z=(key[2]+.5)*SliceMesh::width-eye[axes[1]];
            return a*a+x*x+z*z;
        };
        for(int budget=0;budget<4 && pending.size()<16;++budget) {
            auto best=dirty.end();double nearest=1e300;
            for(auto it=dirty.begin();it!=dirty.end();++it)if(!pending.contains(*it) && wanted.contains(*it)){
                double d=distance(*it);if(d<nearest){best=it;nearest=d;}
            }
            if(best==dirty.end())break;
            auto key=*best;dirty.erase(best);
            SliceMesh::Snapshot snapshot;snapshot.origin={key[0]*32,key[1]*SliceMesh::width,key[2]*SliceMesh::width};
            snapshot.axes={0,axes[0],axes[1]};
            // Copy immutable block IDs, including the one-cell neighbor border.
            for(int z=-1;z<=SliceMesh::width;++z)for(int x=-1;x<=SliceMesh::width;++x) {
                auto p=slice;p[axes[0]]=snapshot.origin[1]+x;p[axes[1]]=snapshot.origin[2]+z;
                p[0]=key[0];auto center=columns.find(p);
                for(int a=0;a<32;++a)
                    snapshot.at(a,x,z)=center==columns.end()?65535:center->second.blocks[a];
                --p[0];auto below=columns.find(p);
                snapshot.at(-1,x,z)=below==columns.end()?65535:below->second.blocks[31];
                p[0]+=2;auto above=columns.find(p);
                snapshot.at(32,x,z)=above==columns.end()?65535:above->second.blocks[0];
            }
            pending.emplace(key,BackgroundWork::rendering().addTaskFuture([snapshot=std::move(snapshot),opacity=opaque,show=showOpenDoor] {
                return SliceMesh::build(snapshot,*opacity,show);
            }));
        }
    }
    void draw(Shader& shader,const Vector& eye,const Vector& forward,const Vector& right,const Vector& up,float aspect) {
        shader.use();shader.setUniform("meshAxes",2,axes.data());shader.setUniform("aspect",aspect);shader.setUniform("lineMode",false);
        auto vector=[&](const char* name,const Vector& v){shader.setUniform(name,glm::vec3(float(v[0]),float(v[axes[0]]),float(v[axes[1]])));};
        vector("meshEye",eye);vector("meshForward",forward);vector("meshRight",right);vector("meshUp",up);
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);
        for(auto& [key,tile]:tiles)if(tile.count){bind(tile.buffer);glDrawArrays(GL_TRIANGLES,0,tile.count);}
        glBindVertexArray(0);
    }
    void drawBox(Shader& shader,const Vector& eye,const Position& low,const Position& high) {
        for(size_t i=1;i<N;++i)if(int(i)!=axes[0] && int(i)!=axes[1] && (eye[i]<low[i] || eye[i]>=high[i]))return;
        std::vector<SliceMesh::Vertex> lines;
        int dims[]{0,axes[0],axes[1]};
        for(int corner=0;corner<8;++corner)for(int axis=0;axis<3;++axis)if(!(corner&(1<<axis))) {
            for(int endpoint:{corner,corner|(1<<axis)}) {
                SliceMesh::Vertex v{};for(int i=0;i<3;++i)v.position[i]=float((endpoint&(1<<i))?high[dims[i]]:low[dims[i]]);
                lines.push_back(v);
            }
        }
        if(!lineBuffer)glGenBuffers(1,&lineBuffer);
        bind(lineBuffer);glBufferData(GL_ARRAY_BUFFER,lines.size()*sizeof(SliceMesh::Vertex),lines.data(),GL_STREAM_DRAW);
        shader.setUniform("lineMode",true);glDepthFunc(GL_LEQUAL);glDrawArrays(GL_LINES,0,GLsizei(lines.size()));
        shader.setUniform("lineMode",false);glBindVertexArray(0);
    }
};

#pragma once
#include "Block.h"
#include <array>
#include <cstdint>
#include <vector>

namespace SliceMesh {
    inline constexpr int width=4,height=32,padded=width+2;
    struct Vertex {
        float position[3];
        int32_t cell[3];
        uint32_t id,side;
    };
    struct Snapshot {
        std::array<int,3> origin{},axes{};
        std::array<uint16_t,(height+2)*padded*padded> blocks{};
        uint16_t& at(int a,int x,int z){return blocks[(a+1)+ (height+2)*((x+1)+padded*(z+1))];}
        uint16_t at(int a,int x,int z) const{return blocks[(a+1)+ (height+2)*((x+1)+padded*(z+1))];}
    };
    inline bool visible(uint16_t id){return id!=Block::AIR && id!=65535 && id!=ExtendedBlock::CRYSTAL_DOOR_OPEN;}
    inline std::vector<Vertex> build(const Snapshot& s,const std::vector<uint8_t>& opaque,bool showOpenDoor=false) {
        std::vector<Vertex> vertices;
        constexpr int triangles[]{0,1,2,0,2,3};
        constexpr int corners[4][2]{{0,0},{1,0},{1,1},{0,1}};
        for(int z=0;z<width;++z)for(int x=0;x<width;++x)for(int a=0;a<height;++a) {
            uint16_t id=s.at(a,x,z);if(!visible(id) && !(showOpenDoor && id==ExtendedBlock::CRYSTAL_DOOR_OPEN))continue;
            int cell[]{a,x,z};
            for(int normal=0;normal<3;++normal)for(int sign=0;sign<2;++sign) {
                int n[]{a,x,z};n[normal]+=sign?1:-1;
                auto neighbor=s.at(n[0],n[1],n[2]);
                if(neighbor<opaque.size() && opaque[neighbor])continue;
                int u=(normal+1)%3,v=(normal+2)%3;
                for(int corner:triangles) {
                    Vertex vertex{};
                    for(int axis=0;axis<3;++axis)vertex.cell[axis]=s.origin[axis]+cell[axis];
                    for(int axis=0;axis<3;++axis)vertex.position[axis]=float(vertex.cell[axis]);
                    vertex.position[normal]+=float(sign);
                    vertex.position[u]+=float(corners[corner][0]);vertex.position[v]+=float(corners[corner][1]);
                    vertex.id=id;vertex.side=s.axes[normal]*2+sign;vertices.push_back(vertex);
                }
            }
        }
        return vertices;
    }
}

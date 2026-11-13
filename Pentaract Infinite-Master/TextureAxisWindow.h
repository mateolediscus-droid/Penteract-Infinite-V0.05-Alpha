#pragma once
#include "AxisGroups.h"
#include <cmath>

template<size_t N> struct TextureAxisWindow {
    std::vector<int> axes;
    std::array<uint32_t,N> cellHashes{};
    std::array<int,N> cells{},subcells{};
    std::array<bool,N> valid{};
    static uint32_t hash(uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;return x^(x>>16);}
    void update(const AxisGroups& groups,int current,const std::array<double,N>& eye,
        const std::array<double,N>& x,const std::array<double,N>& z){
        std::array<bool,N> selected{};
        for(int offset=0;offset<std::min(3,int(groups.groups.size()));++offset)
            for(int axis:groups.groups[(current+offset)%groups.groups.size()])selected[axis]=true;
        selected[0]=true;
        axes.clear();
        for(size_t axis=0;axis<N;++axis){
            if(x[axis]!=0 || z[axis]!=0)selected[axis]=true;
            if(!selected[axis])continue;
            axes.push_back(int(axis));
            // Match the shader's float coordinates and 16 subcells exactly.
            float point=float(eye[axis]);int cell=int(std::floor(point));
            if(!valid[axis] || cells[axis]!=cell){cells[axis]=cell;cellHashes[axis]=hash(uint32_t(cell));valid[axis]=true;}
            subcells[axis]=int(std::floor(std::clamp(point-float(cell),0.f,.99999f)*16.f));
        }
    }
};

#pragma once
#include "AxisGroups.h"
#include <cmath>

template<size_t N> struct RayAxisGroup {
    std::vector<int> axes;
    std::array<bool,N> active{};
    std::array<int32_t,N> slice{};
    bool update(const AxisGroups& groups,int current,std::array<int,2> plane,const std::array<double,N>& eye){
        auto previous=axes;auto oldSlice=slice;
        active={};active[0]=true;
        for(int axis:groups.groups.at(current))active[axis]=true;
        // Tiny/vertical-only groups still need a usable 3D plane.
        auto fallback=groups.plane(current,int(N));
        if(groups.groups[current].size()<3){active[fallback[0]]=true;active[fallback[1]]=true;}
        axes.clear();bool changed=false;
        for(size_t i=0;i<N;++i){
            slice[i]=int32_t(std::floor(eye[i]));
            if(active[i])axes.push_back(int(i));else changed|=slice[i]!=oldSlice[i];
        }
        return changed || axes!=previous;
    }
    bool contains(const std::array<int32_t,N>& key)const{
        for(size_t i=1;i<N;++i)if(!active[i] && key[i]!=slice[i])return false;
        return true;
    }
    uint32_t hash(const std::array<int32_t,N>& key)const{
        uint32_t value=2166136261u;
        for(int axis:axes)value=(value^uint32_t(key[axis]))*16777619u;
        return value;
    }
};

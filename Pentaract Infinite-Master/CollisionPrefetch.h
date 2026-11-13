#pragma once
#include <array>
#include <cstddef>
#include <vector>
#include <set>
#include <cstdint>
#include <utility>

template<size_t N> class CollisionPrefetch {
    using Position=std::array<int32_t,N>;
    std::vector<Position> previousCore,cached;
    std::vector<int> previousAxes;
    int previousLow=0,previousHigh=-1;
public:
    size_t rebuilds=0;
    const std::vector<Position>& update(std::vector<Position> core,int low,int high,const std::vector<int>& axes){
        if(low==previousLow && high==previousHigh && core==previousCore && axes==previousAxes)return cached;
        std::set<Position> wanted;
        for(auto p:core)for(int a=low;a<=high;++a){
            p[0]=a;wanted.insert(p);
            for(int axis:axes)if(axis)for(int sign:{-1,1}){
                auto neighbor=p;neighbor[axis]+=sign;wanted.insert(neighbor);
            }
        }
        previousCore=std::move(core);previousAxes=axes;previousLow=low;previousHigh=high;
        cached.assign(wanted.begin(),wanted.end());++rebuilds;return cached;
    }
};

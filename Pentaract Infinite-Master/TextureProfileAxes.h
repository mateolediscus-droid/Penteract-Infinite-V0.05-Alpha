#pragma once
#include "AxisGroups.h"
#include <cmath>

// A stays vertical; the authored X/Z plane follows the visible spatial plane.
template<size_t N> std::array<int,5> textureProfileAxes(const AxisGroups& groups,int group,
    const std::array<double,N>& x,const std::array<double,N>& z) {
    static_assert(N>=5);
    std::array<int,5> axes{0,-1,-1,-1,-1};
    for(int slot=1;slot<=2;++slot) {
        const auto& direction=slot==1?x:z;
        double best=-1;
        for(int a=1;a<int(N);++a)if(a!=axes[1] && std::abs(direction[a])>best){
            best=std::abs(direction[a]);axes[slot]=a;
        }
    }
    int next=3;
    auto add=[&](int a){if(next<5 && std::find(axes.begin(),axes.end(),a)==axes.end())axes[next++]=a;};
    for(int a:groups.groups.at(group))add(a);
    for(int a=1;next<5 && a<int(N);++a)add(a);
    return axes;
}

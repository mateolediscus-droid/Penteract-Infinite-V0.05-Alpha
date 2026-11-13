#pragma once
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace SliceColumns
{
    using Point=std::array<double,2>;
    template<size_t N>
    std::array<int,2> alignedAxes(const std::array<double,N>& x,const std::array<double,N>& z) {
        std::array<int,2> axes{-1,-1};
        for(size_t i=1;i<N;++i) {
            if(std::abs(x[i])>1e-10) {if(std::abs(std::abs(x[i])-1.)>1e-10 || axes[0]>=0)return {-1,-1};axes[0]=int(i);}
            if(std::abs(z[i])>1e-10) {if(std::abs(std::abs(z[i])-1.)>1e-10 || axes[1]>=0)return {-1,-1};axes[1]=int(i);}
        }
        if(axes[0]==axes[1])return {-1,-1};
        return axes;
    }
    template<size_t N>
    bool intersects(const std::array<double,N>& eye,const std::array<double,N>& x,
        const std::array<double,N>& z,const std::array<int32_t,N>& cell,double radius,bool touching=false)
    {
        std::array<Point,2*N+4> polygon{},clipped{};
        polygon[0]={-radius,-radius};polygon[1]={radius,-radius};
        polygon[2]={radius,radius};polygon[3]={-radius,radius};
        size_t count=4;
        for(size_t axis=1;axis<N;++axis) {
            if(std::abs(x[axis])+std::abs(z[axis])<1e-10) {
                if(int(std::floor(eye[axis]))!=cell[axis])return false;
                continue;
            }
            for(int side=0;side<2;++side) {
                double sign=side?1.:-1.;
                double limit=side?double(cell[axis])+1-eye[axis]:eye[axis]-cell[axis];
                auto value=[&](Point p){return sign*(x[axis]*p[0]+z[axis]*p[1])-limit;};
                size_t output=0;
                for(size_t i=0;i<count;++i) {
                    auto a=polygon[i],b=polygon[(i+1)%count];double da=value(a),db=value(b);
                    if(da<=0)clipped[output++]=a;
                    if((da<0 && db>0)||(da>0 && db<0)) {
                        double t=da/(da-db);clipped[output++]={a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1])};
                    }
                }
                polygon.swap(clipped);count=output;if(count==0 || (!touching && count<3))return false;
            }
        }
        double area=0;
        for(size_t i=0;i<count;++i){auto a=polygon[i],b=polygon[(i+1)%count];area+=a[0]*b[1]-a[1]*b[0];}
        return touching || std::abs(area)>1e-10;
    }
    template<size_t N>
    std::vector<std::array<int32_t,N>> enumerate(const std::array<double,N>& eye,
        const std::array<double,N>& x,const std::array<double,N>& z,double radius)
    {
        using Cell=std::array<int32_t,N>;
        Cell start{};for(size_t i=1;i<N;++i)start[i]=(int32_t)std::floor(eye[i]);
        auto axes=alignedAxes(x,z);
        if(axes[0]>=0 && axes[1]>=0) {
            std::vector<Cell> result;
            int loX=int(std::floor(eye[axes[0]]-radius)),hiX=int(std::ceil(eye[axes[0]]+radius))-1;
            int loZ=int(std::floor(eye[axes[1]]-radius)),hiZ=int(std::ceil(eye[axes[1]]+radius))-1;
            result.reserve(size_t(hiX-loX+1)*size_t(hiZ-loZ+1));
            for(int a=loX;a<=hiX;++a)for(int b=loZ;b<=hiZ;++b) {
                auto cell=start;cell[axes[0]]=a;cell[axes[1]]=b;result.push_back(cell);
            }
            return result;
        }
        struct CellHash {
            size_t operator()(const Cell& p) const {
                size_t h=2166136261u;for(auto n:p)h=(h^uint32_t(n))*16777619u;return h;
            }
        };
        std::unordered_set<Cell,CellHash> visited;std::vector<Cell> queue{start},result;
        visited.reserve(size_t((2*radius+3)*(2*radius+3)*N));
        visited.insert(start);
        for(size_t at=0;at<queue.size();++at) {
            auto cell=queue[at];bool inside=intersects(eye,x,z,cell,radius);
            if(inside)result.push_back(cell);
            // Also seed neighbors at a degenerate starting corner.
            if(!inside && at!=0 && !intersects(eye,x,z,cell,radius,true))continue;
            for(size_t i=1;i<N;++i)for(int sign:{-1,1}) {
                if(std::abs(x[i])+std::abs(z[i])<1e-10)continue;
                auto next=cell;next[i]+=sign;
                if(visited.insert(next).second)queue.push_back(next);
            }
        }
        return result;
    }
    template<size_t N> uint32_t hash(const std::array<int32_t,N>& p) {
        uint32_t h=2166136261u;for(auto n:p)h=(h^uint32_t(n))*16777619u;return h;
    }
}

#pragma once
#include <array>
#include <vector>
#include <set>
#include <cmath>
#include <algorithm>
#include <limits>

namespace SideViewND
{
    template<size_t N> using Vector = std::array<double,N>;
    template<size_t N> using Basis = std::array<Vector<N>,N-1>;

    template<size_t N> void align(Basis<N>& basis,int axis,int dimensions=N)
    {
        for(auto& row:basis)row.fill(0);
        basis[0][axis]=1;
        int next=1;
        for(int i=1;i<dimensions;++i) if(i!=axis) basis[next++][i]=1;
    }
    template<size_t N> void rotate(Basis<N>& basis,int hidden,double angle)
    {
        auto visible=basis[0],other=basis[hidden];
        double c=std::cos(angle),s=std::sin(angle);
        for(size_t i=1;i<N;++i) {
            basis[0][i]=visible[i]*c+other[i]*s;
            basis[hidden][i]=other[i]*c-visible[i]*s;
        }
    }
    template<size_t N> bool valid(const Basis<N>& basis,int dimensions=N)
    {
        for(int a=0;a<dimensions-1;++a) {
            for(size_t i=0;i<N;++i) if(!std::isfinite(basis[a][i]) || ((i==0 || i>=dimensions) && basis[a][i]!=0)) return false;
            for(int b=0;b<dimensions-1;++b) {
                double dot=0;for(size_t i=1;i<N;++i)dot+=basis[a][i]*basis[b][i];
                if(std::abs(dot-(a==b?1.:0.))>1e-6)return false;
            }
        }
        return true;
    }
    inline std::array<double,2> halfSize(int width,int height)
    {
        double aspect=double(std::max(width,1))/std::max(height,1);
        double h=std::min(9.,14./aspect);return {h*aspect,h};
    }
    template<size_t N> Vector<N> point(Vector<N> eye,const Vector<N>& right,double mx,double my,int w,int h)
    {
        auto half=halfSize(w,h);
        for(size_t i=1;i<N;++i)eye[i]+=right[i]*(2.*mx/std::max(w,1)-1.)*half[0];
        eye[0]+=(1.-2.*my/std::max(h,1))*half[1];return eye;
    }
    // Traverse only the line's voxels, not an exponentially growing N-D box.
    template<size_t N> std::vector<std::array<int,N>> columns(const Vector<N>& eye,const Vector<N>& right,double radius)
    {
        std::array<int,N> p{},step{};Vector<N> next{},delta{};
        for(size_t i=1;i<N;++i) {
            double start=eye[i]-right[i]*radius;p[i]=int(std::floor(start));
            step[i]=right[i]>0?1:right[i]<0?-1:0;
            delta[i]=step[i]?std::abs(1./right[i]):std::numeric_limits<double>::infinity();
            next[i]=step[i]?((step[i]>0?p[i]+1.-start:start-p[i])*delta[i]):delta[i];
        }
        std::vector<std::array<int,N>> result;
        for(;;) {
            result.push_back(p);
            double t=std::numeric_limits<double>::infinity();
            for(size_t i=1;i<N;++i)t=std::min(t,next[i]);
            if(t>radius*2. || !std::isfinite(t))break;
            for(size_t i=1;i<N;++i)if(next[i]<=t+1e-10){p[i]+=step[i];next[i]+=delta[i];}
        }
        return result;
    }
    template<size_t N> std::vector<std::array<int,N>> collisionChunks(const Vector<N>& eye,const Vector<N>& right)
    {
        std::set<std::array<int,N>> result;
        for(auto p:columns(eye,right,1.25))for(int a=int(std::floor((eye[0]-2.65)/32.));a<=int(std::floor((eye[0]+1.1)/32.));++a) {
            p[0]=a;result.insert(p);
            for(size_t i=1;i<N;++i)for(int sign:{-1,1}){auto q=p;q[i]+=sign;result.insert(q);}
        }
        return {result.begin(),result.end()};
    }
    template<size_t N> bool overlaps(const Vector<N>& eye,const std::array<int,N>& block,const Vector<N>& right)
    {
        if(eye[0]+.1<=block[0] || eye[0]-1.65>=block[0]+1.)return false;
        double low=-.25,high=.25;
        for(size_t i=1;i<N;++i) {
            if(std::abs(right[i])<1e-12) {if(int(std::floor(eye[i]))!=block[i])return false;continue;}
            double a=(block[i]-eye[i])/right[i],b=(block[i]+1.-eye[i])/right[i];
            low=std::max(low,std::min(a,b));high=std::min(high,std::max(a,b));
        }
        return high>low;
    }
    template<size_t N> int side(const Vector<N>& point,const Vector<N>& right)
    {
        double best=1e30;int side=0;
        for(size_t i=0;i<N;++i) {
            double weight=i==0?1.:std::abs(right[i]);if(weight<1e-8)continue;
            double f=point[i]-std::floor(point[i]);double distance=std::min(f,1.-f)/weight;
            if(distance<best){best=distance;side=int(i)*2+(f>=.5);}
        }
        return side;
    }
    // Actual keyboard labels: visible Q/D, then Z/S, A/E, R/F, T/G, Y/H.
    inline int control(char label,int count=12)
    {
        const char* pairs="qdszaerftgyhikoluj";
        for(int i=0;i<std::min(count,18);++i)if(pairs[i]==label)return i;
        return -1;
    }
}

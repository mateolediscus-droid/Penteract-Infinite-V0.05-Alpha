#pragma once
#include <array>
#include <cmath>
#include <stdexcept>

// A hidden direction chosen in the current frame, then transported with it.
template<size_t N> class RotationTarget {
    using Vector=std::array<double,N>;
    using Basis=std::array<Vector,N-1>;
    Vector direction{};
    Basis previous{};
    int selected=-1;
    static Vector hidden(int axis,const Basis& basis,int renderType) {
        Vector result{};result[axis]=1;
        for(int visible=0;visible<(renderType==2?1:2);++visible)
            for(size_t i=1;i<N;++i)result[i]-=basis[visible][axis]*basis[visible][i];
        double length=0;for(double x:result)length+=x*x;
        if(length<1e-12)throw std::runtime_error("Axis is already in the visible plane; choose a hidden direction.");
        length=std::sqrt(length);for(auto& x:result)x/=length;
        return result;
    }
public:
    int axis()const{return selected;}
    void choose(int axis,const Basis& basis,int renderType) {
        if(axis<0){selected=-1;return;}
        if(axis==0 || axis>=N)throw std::runtime_error("Choose a horizontal axis.");
        auto next=hidden(axis,basis,renderType);
        direction=next;previous=basis;selected=axis;
    }
    bool turn(Basis& basis,int renderType,double yaw,double dx,double dy) {
        if(selected<0)return false;
        if(basis!=previous) {
            try{direction=hidden(selected,basis,renderType);}
            catch(const std::runtime_error&){return true;}
        }
        auto rotate=[&](Vector a,Vector b,double angle) {
            double c=std::cos(angle),s=std::sin(angle);
            auto apply=[&](Vector& v) {
                double x=0,y=0;for(size_t i=1;i<N;++i){x+=v[i]*a[i];y+=v[i]*b[i];}
                for(size_t i=1;i<N;++i)v[i]+=a[i]*(x*(c-1)-y*s)+b[i]*(x*s+y*(c-1));
            };
            for(auto& v:basis)apply(v);apply(direction);
        };
        if(renderType==2)rotate(basis[0],direction,dx);
        else {
            Vector lateral{},heading{};
            for(size_t i=1;i<N;++i)lateral[i]=-basis[0][i]*std::sin(yaw)+basis[1][i]*std::cos(yaw);
            rotate(lateral,direction,dx);
            for(size_t i=1;i<N;++i)heading[i]=basis[0][i]*std::cos(yaw)+basis[1][i]*std::sin(yaw);
            rotate(heading,direction,dy);
        }
        previous=basis;return true;
    }
};

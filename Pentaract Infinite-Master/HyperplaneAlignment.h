#pragma once
#include <cmath>

namespace HyperplaneAlignment {
template<class Position> void centerHorizontal(Position& position,int dimensions){
    for(int axis=1;axis<dimensions;++axis)position[axis]=std::floor(position[axis])+0.5;
}
}

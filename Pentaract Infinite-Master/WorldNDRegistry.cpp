#include "WorldNDRegistry.h"
#include <stdexcept>
#define WORLD_DIMENSION(N) WorldNDEntry registeredWorld##N();
#include "WorldNDDimensions.inc"
#undef WORLD_DIMENSION
#ifdef ND_RUNTIME_DIMENSION
WorldNDEntry registeredRuntimeWorld();
#endif

bool worldNDCompiled(int dimensions){
    if(dimensions>=2 && dimensions<=33)return true;
#ifdef ND_RUNTIME_DIMENSION
    return dimensions==ND_RUNTIME_DIMENSION;
#else
    return false;
#endif
}

WorldNDEntry worldNDEntry(int dimensions){
    switch(dimensions){
#define WORLD_DIMENSION(N) case N:return registeredWorld##N();
#include "WorldNDDimensions.inc"
#undef WORLD_DIMENSION
#ifdef ND_RUNTIME_DIMENSION
    case ND_RUNTIME_DIMENSION:return registeredRuntimeWorld();
#endif
    default:throw std::invalid_argument("This dimension requires compilation.");
    }
}

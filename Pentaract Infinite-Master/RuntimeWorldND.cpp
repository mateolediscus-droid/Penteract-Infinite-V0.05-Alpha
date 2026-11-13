#include "WorldNDRegistration.h"
#ifndef ND_RUNTIME_DIMENSION
#error ND_RUNTIME_DIMENSION is required
#endif
static_assert(ND_RUNTIME_DIMENSION>33);
template class StateGameND<ND_RUNTIME_DIMENSION>;
WorldNDEntry registeredRuntimeWorld(){return registerWorldND<ND_RUNTIME_DIMENSION>();}

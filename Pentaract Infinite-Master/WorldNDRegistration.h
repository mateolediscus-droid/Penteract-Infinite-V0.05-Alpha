#pragma once
#include "WorldNDRegistry.h"
#include "StateGameND.h"

template<int N> WorldNDEntry registerWorldND(){
    return {[]{return static_cast<State*>(StateGameND<N>::instance());},
        [](const std::filesystem::path& path,uint64_t seed,bool flat,bool caves){
            StateGameND<N>::instance()->configure(path,seed,flat,caves);
        }};
}

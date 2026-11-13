#pragma once
#include <cstdint>
#include <limits>

namespace WaterSettings {
inline constexpr uint64_t unlimited=std::numeric_limits<uint64_t>::max();
inline uint64_t maxUpdates=300;
inline uint64_t revision=0;
inline void set(uint64_t value){maxUpdates=value;++revision;}
}

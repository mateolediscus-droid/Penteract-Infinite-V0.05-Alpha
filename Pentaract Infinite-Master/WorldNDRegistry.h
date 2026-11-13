#pragma once
#include "State.h"
#include <filesystem>
#include <cstdint>

// Keep dimension arithmetic (including face counts) within signed int range.
inline constexpr int maxSpatialDimensions=1073741791;
struct WorldNDEntry {
    State* (*instance)();
    void (*configure)(const std::filesystem::path&,uint64_t,bool,bool);
};
WorldNDEntry worldNDEntry(int dimensions);
bool worldNDCompiled(int dimensions);

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Mirrors the std430 NDFrameData block in both ND fragment shaders.
template<std::size_t N> struct NDFrameData {
    static_assert(N>=6);
    std::array<float,N> eye{},forward{},right{},up{};
    std::array<int32_t,N-5> textureExtraAxes{};
    std::array<int32_t,N> lookupSlice{},target{};
    std::array<float,N> boxMin{},boxMax{};
    std::array<int32_t,N> textureWindowAxes{};
    std::array<uint32_t,N> textureCachedCell{};
    std::array<int32_t,N> textureCachedSub{};
};

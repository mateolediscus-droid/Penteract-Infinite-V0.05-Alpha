#pragma once
#include <algorithm>

inline constexpr int maxTemporalDimensions(int spatialDimensions)
{
    return spatialDimensions >= 2 ? 2 : 1;
}

inline constexpr int temporalDimensionsFor(int spatialDimensions, int requested)
{
    return std::clamp(requested, 1, maxTemporalDimensions(spatialDimensions));
}

#pragma once

inline int worldRenderType(int dimensions, int requested = 3)
{
    if (dimensions == 2) return 2;
    return requested == 2 ? 2 : 3;
}

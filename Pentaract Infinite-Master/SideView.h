#pragma once
#include "Math5D.h"

namespace SideView
{
    inline m5::vec5 right(float angle)
    {
        float c = std::cos(angle), s = std::sin(angle);
        // Exact cardinal directions prevent floor()/DDA disagreement at voxel boundaries.
        if (std::abs(c) < 0.000001f) { c = 0.0f; s = s < 0 ? -1.0f : 1.0f; }
        if (std::abs(s) < 0.000001f) { s = 0.0f; c = c < 0 ? -1.0f : 1.0f; }
        return {0.0f,c,s,0.0f,0.0f};
    }

    inline glm::vec2 halfSize(int width, int height)
    {
        float aspect = float(glm::max(width, 1)) / float(glm::max(height, 1));
        float halfHeight = glm::min(9.0f, 14.0f / aspect);
        return {halfHeight * aspect, halfHeight};
    }

    inline m5::vec5 point(m5::vec5 eye, glm::dvec2 mouse, int width, int height,
        m5::vec5 right = {0,1,0,0,0})
    {
        auto half = halfSize(width, height);
        auto result = eye + right * float(2.0 * mouse.x / glm::max(width, 1) - 1.0) * half.x;
        result.a += float(1.0 - 2.0 * mouse.y / glm::max(height, 1)) * half.y;
        return result;
    }
}

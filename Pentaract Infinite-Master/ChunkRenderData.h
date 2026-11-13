#pragma once
#include "Chunk.h"
#include <vector>

namespace ChunkRendering
{
    constexpr int SECTIONS = Chunk::RENDER_SECTIONS;
    constexpr int SECTION_HEIGHT = Chunk::HEIGHT / SECTIONS;
    constexpr int VERTICAL_TEXELS = SECTION_HEIGHT / 8;
    constexpr int WORDS = Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * VERTICAL_TEXELS * 4;

    struct PackedChunk
    {
        std::vector<uint32_t> blocks;
        bool empty = true;
    };

    inline PackedChunk pack(const Chunk& chunk, int section)
    {
        PackedChunk result{ std::vector<uint32_t>(WORDS, 0), true };
        for (int b = 0; b < Chunk::SIZE; ++b)
        for (int c = 0; c < Chunk::SIZE; ++c)
        for (int d = 0; d < Chunk::SIZE; ++d)
        for (int e = 0; e < Chunk::SIZE; ++e)
        for (int a = 0; a < SECTION_HEIGHT; ++a)
        {
            uint16_t id = chunk.getBlockFull({
                (uint32_t)(section * SECTION_HEIGHT + a), (uint32_t)b,
                (uint32_t)c, (uint32_t)d, (uint32_t)e });
            if (!ExtendedBlock::isRenderable(id)) continue;
            result.empty = false;
            int z = (d + e * Chunk::SIZE) * VERTICAL_TEXELS + a / 8;
            int texel = b + c * Chunk::SIZE + z * Chunk::SIZE * Chunk::SIZE;
            result.blocks[texel * 4 + (a % 8) / 2] |= (uint32_t)id << ((a % 2) * 16);
        }
        return result;
    }
}

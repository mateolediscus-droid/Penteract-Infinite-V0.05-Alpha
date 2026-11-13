#pragma once
#include "Chunk.h"
#include <bit>
#include <utility>

// Shared by 6D/7D. Horizontal cells are all one voxel wide; A stays 32 high.
struct ColumnChunk
{
    std::array<uint16_t,32> blocks{};
    uint32_t extendedMask=0;
    std::atomic<bool> shouldUpdateRenderer[1]{true};
    struct ExtendedEntries {
        const ColumnChunk* chunk;
        struct Iterator {
            const ColumnChunk* chunk;
            uint32_t remaining;
            auto operator*() const {
                unsigned a=std::countr_zero(remaining);
                return std::pair<uint32_t,uint16_t>{a,chunk->blocks[a]};
            }
            Iterator& operator++(){remaining&=remaining-1;return *this;}
            bool operator==(const Iterator&) const=default;
        };
        Iterator begin() const{return {chunk,chunk->extendedMask};}
        Iterator end() const{return {chunk,0};}
    };
    ExtendedEntries extendedBlocks() const{return {this};}
    uint16_t getBlockFull(const m5::uvec5& p) const {return blocks.at(p.a);}
    void setBlock(const m5::uvec5& p,uint16_t id) {
        blocks.at(p.a)=id;
        if(ExtendedBlock::isExtended(id) && id!=ExtendedBlock::DEEPSTONE)extendedMask|=1u<<p.a;
        else extendedMask&=~(1u<<p.a);
        shouldUpdateRenderer[0]=true;
    }
};

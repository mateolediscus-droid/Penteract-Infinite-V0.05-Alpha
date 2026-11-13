#pragma once
#include "MashPool.h"
#include <algorithm>

namespace BackgroundWork
{
    constexpr unsigned generationThreads(unsigned hardwareThreads)
    {
        const unsigned previous = std::clamp(hardwareThreads / 2, 1u, 4u);
        // Keep capacity for the main thread and other background work.
        return hardwareThreads > previous + 2 ? previous + 1 : previous;
    }
    static_assert(generationThreads(0) == 1 && generationThreads(2) == 1);
    static_assert(generationThreads(4) == 2 && generationThreads(8) == 5);
    static_assert(generationThreads(16) == 5);

    inline MashPool& storage()
    {
        static MashPool pool(1);
        return pool;
    }
    inline MashPool& generation()
    {
        static MashPool pool(generationThreads(std::thread::hardware_concurrency()));
        return pool;
    }

    inline MashPool& rendering()
    {
        static MashPool pool(std::thread::hardware_concurrency() >= 6 ? 2 : 1);
        return pool;
    }
}

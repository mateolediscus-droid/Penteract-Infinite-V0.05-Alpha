#pragma once
#include "../ColumnChunk.h"
#include "../NoiseND.h"
#include "../SliceColumns.h"
#include "BackgroundWork.h"
#include <array>
#include <cmath>
#include <functional>
#include <future>
#include <map>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include "TemporalEvents.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include "json.hpp"
#include "../../FaceMaskND.h"

// One implementation, specialized by the compiler. No per-cell heap coordinates.
template<int N> struct WorldND {
    static_assert(N>=6);
    using FaceMask=FaceMaskND<N>;
    static constexpr int saveVersion=N<=15?2:3;
    static std::string structureMagic(){return N<=15?(N<10?"PSTR":"PST")+std::to_string(N)+"D\r\n":"PSTRND\r\n";}
#include "Space.inl"
#include "Logic.inl"
#include "Structures.inl"
#include "Terrain.inl"
#include "Generation.inl"
#include "Camera.inl"
#include "Falling.inl"
};

#pragma once
#include "../nd/WorldND.h"
namespace world14d {
using Engine=WorldND<14>;
using Position=Engine::Position;
using Vector=Engine::Vector;
using Address=Engine::Address;
using Hit=Engine::Hit;
using Space=Engine::Space;
using BlockState=Engine::BlockState;
using Logic=Engine::Logic;
using Terrain=Engine::Terrain;
using StructureRule=Engine::StructureRule;
using Generation=Engine::Generation;
using CameraFrame=Engine::CameraFrame;
using Falling=Engine::Falling;
using structures=Engine::structures;
inline constexpr auto extent=Engine::extent;
inline constexpr auto address=&Engine::address;
inline constexpr auto position=&Engine::position;
inline constexpr auto neighbor=&Engine::neighbor;
inline constexpr auto validate=&Engine::validate;
inline bool bodyOverlaps(const Vector& p,const Position& b,const CameraFrame& f={}){return Engine::bodyOverlaps(p,b,f);}
inline auto collisionChunks(const Vector& p,const CameraFrame& f={}){return Engine::collisionChunks(p,f);}
inline auto abovePlacedBlock(Vector p,const Position& b,const CameraFrame& f={}){return Engine::abovePlacedBlock(p,b,f);}
}

#include "World.h"
#include "BackgroundWork.h"
#include "Math5D.h"
#include "OpenSimplexNoise.hpp"
#include <glm/common.hpp>
#include <cstring>
#include <optional>

namespace
{
	double sampleCaveNoise5D(OpenSimplexNoise& noise, const glm::dvec4& worldPos, double worldA, double scale)
	{
		double a = worldA * scale;
		glm::dvec4 p = worldPos * scale;

		double n1 = noise.Evaluate(p.x + a * 0.73, p.y, p.z, p.w);
		double n2 = noise.Evaluate(p.x, p.y - a * 0.61, p.z + a * 0.37, p.w);
		return (n1 + n2) * 0.5;
	}

	bool shouldCarveCave(OpenSimplexNoise& regionNoise, OpenSimplexNoise& shapeNoise, OpenSimplexNoise& detailNoise, const glm::dvec4& worldPos, int worldA)
	{
		if (worldA > 20)
		{
			return false;
		}

		float region = (float)((sampleCaveNoise5D(regionNoise, worldPos, worldA, 0.012) * 0.5) + 0.5);
		if (region < 0.46f)
		{
			return false;
		}

		float shape = (float)((sampleCaveNoise5D(shapeNoise, worldPos, worldA, 0.055) * 0.5) + 0.5);
		float detail = (float)sampleCaveNoise5D(detailNoise, worldPos, worldA, 0.13);
		float depthFade = glm::clamp((20.0f - (float)worldA) / 10.0f, 0.0f, 1.0f);
		float caveField = shape + detail * 0.16f + (region - 0.46f) * 0.35f;
		float threshold = 0.66f - depthFade * 0.08f;

		return caveField > threshold;
	}

	struct CaveCache
	{
		static constexpr int STEP = 2;
		static constexpr int A_SIZE = Chunk::HEIGHT / STEP + 1;
		static constexpr int SIDE_SIZE = Chunk::SIZE / STEP + 1;
		std::vector<float> values;

		CaveCache() : values(A_SIZE * SIDE_SIZE * SIDE_SIZE * SIDE_SIZE * SIDE_SIZE, -1.0f) {}

		static int index(int a, int b, int c, int d, int e)
		{
			return (((a * SIDE_SIZE + b) * SIDE_SIZE + c) * SIDE_SIZE + d) * SIDE_SIZE + e;
		}

		float get(int a, int b, int c, int d, int e) const
		{
			return values[index(a, b, c, d, e)];
		}

		void set(int a, int b, int c, int d, int e, float value)
		{
			values[index(a, b, c, d, e)] = value;
		}
	};

	float caveDensity(OpenSimplexNoise& regionNoise, OpenSimplexNoise& shapeNoise, OpenSimplexNoise& detailNoise, const glm::dvec4& worldPos, int worldA)
	{
		if (worldA > 20)
		{
			return -1.0f;
		}

		float region = (float)((sampleCaveNoise5D(regionNoise, worldPos, worldA, 0.012) * 0.5) + 0.5);
		if (region < 0.46f)
		{
			return -1.0f;
		}

		float shape = (float)((sampleCaveNoise5D(shapeNoise, worldPos, worldA, 0.055) * 0.5) + 0.5);
		float detail = (float)sampleCaveNoise5D(detailNoise, worldPos, worldA, 0.13);
		float depthFade = glm::clamp((20.0f - (float)worldA) / 10.0f, 0.0f, 1.0f);
		float caveField = shape + detail * 0.16f + (region - 0.46f) * 0.35f;
		float threshold = 0.66f - depthFade * 0.08f;

		return caveField - threshold;
	}

	float lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

	float sampleCaveCache(const CaveCache& cache, int a, int b, int c, int d, int e)
	{
		auto split = [](int value, int maxIndex, int& i0, int& i1, float& t)
			{
				i0 = glm::clamp(value / CaveCache::STEP, 0, maxIndex - 1);
				i1 = i0 + 1;
				t = (float)(value - i0 * CaveCache::STEP) / (float)CaveCache::STEP;
			};

		int a0, a1, b0, b1, c0, c1, d0, d1, e0, e1;
		float at, bt, ct, dt, et;
		split(a, CaveCache::A_SIZE - 1, a0, a1, at);
		split(b, CaveCache::SIDE_SIZE - 1, b0, b1, bt);
		split(c, CaveCache::SIDE_SIZE - 1, c0, c1, ct);
		split(d, CaveCache::SIDE_SIZE - 1, d0, d1, dt);
		split(e, CaveCache::SIDE_SIZE - 1, e0, e1, et);

		float aSamples[2]{ 0.0f, 0.0f };
		for (int ia = 0; ia < 2; ++ia)
		{
			int ai = ia ? a1 : a0;
			float bSamples[2]{ 0.0f, 0.0f };
			for (int ib = 0; ib < 2; ++ib)
			{
				int bi = ib ? b1 : b0;
				float cSamples[2]{ 0.0f, 0.0f };
				for (int ic = 0; ic < 2; ++ic)
				{
					int ci = ic ? c1 : c0;
					float dSamples[2]{ 0.0f, 0.0f };
					for (int id = 0; id < 2; ++id)
					{
						int di = id ? d1 : d0;
						dSamples[id] = lerp(cache.get(ai, bi, ci, di, e0), cache.get(ai, bi, ci, di, e1), et);
					}
					cSamples[ic] = lerp(dSamples[0], dSamples[1], dt);
				}
				bSamples[ib] = lerp(cSamples[0], cSamples[1], ct);
			}
			aSamples[ia] = lerp(bSamples[0], bSamples[1], bt);
		}

		return lerp(aSamples[0], aSamples[1], at);
	}

	void setPackedBlock(Chunk::BlockColumn& column, int a, uint8_t blockType)
	{
		int shift = (a & 1) * 4;
		int index = (a >> 1);
		column.blockData[index] &= (uint8_t)~(0xFu << shift);
		column.blockData[index] |= blockType << shift;
	}

	int sideIndex(int axis, int dir)
	{
		return axis * 2 + (dir > 0 ? 1 : 0);
	}

	m5::ivec5 neighborOnSide(m5::ivec5 blockPos, uint8_t side)
	{
		int axis = side / 2;
		int dir = (side & 1) ? 1 : -1;
		blockPos[axis] += dir;
		return blockPos;
	}

	int oppositeSide(uint8_t side)
	{
		return (side & 1) ? side - 1 : side + 1;
	}
}

World::World() : edgeLength{ 0 }
{
}

World::World(uint8_t edgeLength, uint64_t seed, bool flat, bool caves, int dimensions) :
	edgeLength{ edgeLength },
	dimensions{ dimensions },
	seed{ seed },
	flat{ flat },
	caves{ caves }
{
	assert(edgeLength > 0);
	if (dimensions < 2 || dimensions > 5) throw std::invalid_argument("World dimensions must be between 2 and 5");
}

bool World::containsPosition(m5::ivec5 pos) const
{
	for (int axis = dimensions; axis < 5; ++axis) if (pos[axis] != 0) return false;
	return true;
}

size_t World::getEdgeLength() const
{
	return edgeLength;
}

uint64_t World::getSeed() const
{
	return seed;
}

bool World::isFlat() const
{
	return flat;
}

bool World::hasCaves() const
{
	return caves;
}

void World::setGenerator(uint64_t seed, bool flat, bool caves)
{
	pendingChunks.clear();
	this->seed = seed;
	this->flat = flat;
	this->caves = caves;
}

size_t World::getLoadedChunkCount() const
{
	return chunks.size();
}

void World::setStructures(std::shared_ptr<const Structures::Catalog> catalog)
{
	for (const auto& rule : *catalog) Structures::requireDimensions(rule.structure, dimensions);
	if (structures && Structures::toJson(*structures) == Structures::toJson(*catalog)) return;
	pendingChunks.clear();
	structures = std::move(catalog);
	for (auto& [owner, region] : structureRegions) { region.ready = false; region.cursor = region.min; }
	currentStructureVersion = structureHistory.size();
	structureHistory.push_back(structures);
}

std::shared_ptr<const Structures::Catalog> World::structuresFor(m5::ivec5 pos) const
{
	auto it = chunkStructureVersions.find(key(pos));
	return it != chunkStructureVersions.end() ? structureHistory.at(it->second) : structures;
}

void World::rememberStructureVersion(m5::ivec5 pos)
{
	if (structureHistory.empty())
	{
		structures = std::make_shared<Structures::Catalog>();
		structureHistory.push_back(structures);
	}
	chunkStructureVersions.try_emplace(key(pos), currentStructureVersion);
}

void World::useSavedStructureBaseline(m5::ivec5 pos)
{
	if (!structureHistory.empty()) chunkStructureVersions.try_emplace(key(pos), 0);
}

void World::loadStructureHistory(const nlohmann::json& json)
{
	pendingChunks.clear();
	structureHistory.clear();
	chunkStructureVersions.clear();
	if (json.at("version") == 1)
		structureHistory.push_back(std::make_shared<Structures::Catalog>(Structures::catalogFromJson(json)));
	else
	{
		for (const auto& catalog : json.at("catalogs"))
			structureHistory.push_back(std::make_shared<Structures::Catalog>(Structures::catalogFromJson(catalog)));
		for (const auto& record : json.at("chunks"))
		{
			auto p = record.at("pos").get<std::array<int,5>>();
			size_t version = record.at("catalog").get<size_t>();
			if (version >= structureHistory.size()) throw std::runtime_error("Unknown structure catalog version");
			chunkStructureVersions[{p[0],p[1],p[2],p[3],p[4]}] = version;
		}
	}
	if (structureHistory.empty()) structureHistory.push_back(std::make_shared<Structures::Catalog>());
	for (const auto& catalog : structureHistory)
		for (const auto& rule : *catalog) Structures::requireDimensions(rule.structure, dimensions);
	currentStructureVersion = structureHistory.size() - 1;
	structures = structureHistory.back();
}

nlohmann::json World::saveStructureHistory() const
{
	nlohmann::json catalogs = nlohmann::json::array(), assignments = nlohmann::json::array();
	for (const auto& catalog : structureHistory) catalogs.push_back(Structures::toJson(*catalog));
	for (const auto& [k, version] : chunkStructureVersions)
		assignments.push_back({{"pos", {std::get<0>(k),std::get<1>(k),std::get<2>(k),std::get<3>(k),std::get<4>(k)}}, {"catalog", version}});
	return {{"version", 2}, {"catalogs", catalogs}, {"chunks", assignments}};
}

void World::setStructureRegion(m5::ivec5 owner, m5::ivec5 min, m5::ivec5 size)
{
	if (!containsPosition(owner) || !containsPosition(min)) return;
	for (int axis = dimensions; axis < 5; ++axis) size[axis] = 1;
	m5::ivec5 low, high;
	for (int axis = 0; axis < 5; ++axis)
	{
		int extent = axis == 0 ? Chunk::HEIGHT : Chunk::SIZE;
		low[axis] = Structures::floorDiv(min[axis], extent);
		high[axis] = Structures::floorDiv(min[axis] + size[axis] - 1, extent);
	}
	auto it = structureRegions.find(blockKey(owner));
	if (it != structureRegions.end() && it->second.min == low && it->second.max == high) return;
	structureRegions[blockKey(owner)] = {low, high, low, false};
}

void World::updateStructureRegions()
{
	int budget = 8;
	for (auto& [owner, region] : structureRegions)
	{
		while (!region.ready && budget-- > 0)
		{
			requestChunk(region.cursor, true);
			if (!findChunk(region.cursor) && !pendingChunks.contains(key(region.cursor))) return;
			for (int axis = 4; axis >= 0; --axis)
			{
				if (++region.cursor[axis] <= region.max[axis]) break;
				region.cursor[axis] = region.min[axis];
				if (axis == 0) region.ready = true;
			}
		}
		if (budget <= 0) break;
	}
}

int World::getNaturalTerrainMaxA() const
{
	return 127;
}

int World::getNaturalTerrainMaxChunkA() const
{
	return getNaturalTerrainMaxA() / (int)Chunk::HEIGHT;
}

bool World::canGenerateChunk(m5::ivec5 chunkPos) const
{
	if (!containsPosition(chunkPos)) return false;
	auto structures = structuresFor(chunkPos);
	if (chunkPos.a < 0)
	{
		return true;
	}
	int top = getNaturalTerrainMaxA();
	if (structures)
		for (const auto& rule : *structures)
			top = std::max(top, std::min(rule.maxHeight, getNaturalTerrainMaxA()) + rule.structure.size.a);
	return chunkPos.a <= top / (int)Chunk::HEIGHT;
}

World::ChunkKey World::key(m5::ivec5 chunkPos) const
{
	return { chunkPos.a, chunkPos.b, chunkPos.c, chunkPos.d, chunkPos.e };
}

World::ChunkKey World::blockKey(m5::ivec5 blockPos) const
{
	return { blockPos.a, blockPos.b, blockPos.c, blockPos.d, blockPos.e };
}

void World::wakeFallingSandAt(m5::ivec5 blockPos)
{
	simulation.activeSandBlocks.insert(blockKey(blockPos));
}

void World::wakeFallingSandAbove(m5::ivec5 blockPos)
{
	++blockPos.a;
	wakeFallingSandAt(blockPos);
}

void World::wakeAntiSandAt(m5::ivec5 blockPos)
{
	simulation.activeAntiSandBlocks.insert(blockKey(blockPos));
}

void World::wakeAntiSandBelow(m5::ivec5 blockPos)
{
	--blockPos.a;
	wakeAntiSandAt(blockPos);
}

void World::wakeCableAt(m5::ivec5 blockPos)
{
	simulation.activeCableBlocks.insert(blockKey(blockPos));
}

void World::wakeCableNeighbors(m5::ivec5 blockPos)
{
	for (int axis = 0; axis < dimensions; ++axis)
	{
		m5::ivec5 neighbor = blockPos;
		--neighbor[axis];
		wakeCableAt(neighbor);

		neighbor = blockPos;
		++neighbor[axis];
		wakeCableAt(neighbor);
	}
}

void World::wakeCrystalDoorAt(m5::ivec5 blockPos)
{
	simulation.activeCrystalDoorBlocks.insert(blockKey(blockPos));
}

void World::wakeCrystalDoorNeighbors(m5::ivec5 blockPos)
{
	for (int axis = 0; axis < dimensions; ++axis)
	{
		m5::ivec5 neighbor = blockPos;
		--neighbor[axis];
		wakeCrystalDoorAt(neighbor);

		neighbor = blockPos;
		++neighbor[axis];
		wakeCrystalDoorAt(neighbor);
	}
}

void World::wakeRepeaterAt(m5::ivec5 blockPos)
{
	simulation.activeRepeaterBlocks.insert(blockKey(blockPos));
}

void World::wakeRepeaterNeighbors(m5::ivec5 blockPos)
{
	for (int axis = 0; axis < dimensions; ++axis)
	{
		m5::ivec5 neighbor = blockPos;
		--neighbor[axis];
		wakeRepeaterAt(neighbor);

		neighbor = blockPos;
		++neighbor[axis];
		wakeRepeaterAt(neighbor);
	}
}

void World::wakeRedirectorAt(m5::ivec5 blockPos)
{
	simulation.activeRedirectorBlocks.insert(blockKey(blockPos));
}

void World::wakeRedirectorNeighbors(m5::ivec5 blockPos)
{
	for (int axis = 0; axis < dimensions; ++axis)
	{
		m5::ivec5 neighbor = blockPos;
		--neighbor[axis];
		wakeRedirectorAt(neighbor);

		neighbor = blockPos;
		++neighbor[axis];
		wakeRedirectorAt(neighbor);
	}
}

uint16_t World::getLoadedBlock(const m5::ivec5& block) const
{
	if (!containsPosition(block)) return Block::AIR;
	if (temporalEnabled) return temporalCell(block).id;
	m5::ivec5 chunkPos{
		(int)glm::floor(block.a / (float)Chunk::HEIGHT),
		(int)glm::floor(block.b / (float)Chunk::SIZE),
		(int)glm::floor(block.c / (float)Chunk::SIZE),
		(int)glm::floor(block.d / (float)Chunk::SIZE),
		(int)glm::floor(block.e / (float)Chunk::SIZE)
	};
	const Chunk* c = findChunk(chunkPos);
	if (!c)
	{
		return Block::AIR;
	}

	m5::uvec5 b
	{
		(uint32_t)(block.a - chunkPos.a * Chunk::HEIGHT),
		(uint32_t)(block.b - chunkPos.b * Chunk::SIZE),
		(uint32_t)(block.c - chunkPos.c * Chunk::SIZE),
		(uint32_t)(block.d - chunkPos.d * Chunk::SIZE),
		(uint32_t)(block.e - chunkPos.e * Chunk::SIZE),
	};
	return c->getBlockFull(b);
}

bool World::isBlockLoaded(const m5::ivec5& block) const
{
	if (!containsPosition(block)) return false;
	m5::ivec5 chunkPos{
		(int)glm::floor(block.a / (float)Chunk::HEIGHT),
		(int)glm::floor(block.b / (float)Chunk::SIZE),
		(int)glm::floor(block.c / (float)Chunk::SIZE),
		(int)glm::floor(block.d / (float)Chunk::SIZE),
		(int)glm::floor(block.e / (float)Chunk::SIZE)
	};
	return findChunk(chunkPos) != nullptr;
}

bool World::isRedstoneBlock(uint16_t id) const
{
	return ExtendedBlock::isCable(id) || ExtendedBlock::isLever(id) ||
		ExtendedBlock::isCrystalDoor(id) || ExtendedBlock::isRepeater(id) ||
		ExtendedBlock::isRedirector(id);
}

bool World::hasAdjacentLoadedRedstone(m5::ivec5 blockPos) const
{
	if (!isRedstoneBlock(getLoadedBlock(blockPos)))
	{
		return false;
	}

	for (int axis = 0; axis < dimensions; ++axis)
	{
		for (int dir : { -1, 1 })
		{
			m5::ivec5 neighbor = blockPos;
			neighbor[axis] += dir;
			if (isRedstoneBlock(getLoadedBlock(neighbor)))
			{
				return true;
			}
		}
	}
	return false;
}

void World::activateRedstoneTicksNear(m5::ivec5 blockPos)
{
	if (hasAdjacentLoadedRedstone(blockPos))
	{
		simulation.redstoneTicksActive = true;
		return;
	}

	for (int axis = 0; axis < dimensions; ++axis)
	{
		for (int dir : { -1, 1 })
		{
			m5::ivec5 neighbor = blockPos;
			neighbor[axis] += dir;
			if (hasAdjacentLoadedRedstone(neighbor))
			{
				simulation.redstoneTicksActive = true;
				return;
			}
		}
	}
}

bool World::isRedstonePowered(m5::ivec5 blockPos)
{
	for (int axis = 0; axis < dimensions; ++axis)
	{
		for (int dir : { -1, 1 })
		{
			m5::ivec5 neighbor = blockPos;
			neighbor[axis] += dir;
			uint16_t neighborBlock = getLoadedBlock(neighbor);
			int towardBlock = sideIndex(axis, -dir);
			if (redstoneOutputToward(neighbor, neighborBlock, towardBlock) > 0)
			{
				return true;
			}
		}
	}
	return false;
}

uint8_t World::redstoneOutputToward(m5::ivec5 sourcePos, uint16_t sourceBlock, int towardSide)
{
	if (sourceBlock == ExtendedBlock::LEVER_ON)
	{
		return 15;
	}
	if (ExtendedBlock::isCable(sourceBlock))
	{
		return ExtendedBlock::cablePower(sourceBlock);
	}
	if (ExtendedBlock::isRepeater(sourceBlock))
	{
		auto key = blockKey(sourcePos);
		auto outputIt = simulation.repeaterOutputPowers.find(key);
		if (outputIt == simulation.repeaterOutputPowers.end() || outputIt->second == 0)
		{
			return 0;
		}

		auto inputIt = simulation.repeaterInputFaces.find(key);
		if (inputIt != simulation.repeaterInputFaces.end() && inputIt->second == towardSide)
		{
			return 0;
		}
		return outputIt->second;
	}
	if (ExtendedBlock::isRedirector(sourceBlock))
	{
		if (!ExtendedBlock::redirectorSideOn(sourceBlock, towardSide))
		{
			return 0;
		}

		auto outputIt = simulation.redirectorOutputPowers.find(blockKey(sourcePos));
		return outputIt == simulation.redirectorOutputPowers.end() ? 0 : outputIt->second;
	}
	return 0;
}

Chunk* World::createChunk(m5::ivec5 chunkPos)
{
	// Only immediate physics/edits may wait; rendering uses requestChunk instead.
	auto pending = pendingChunks.find(key(chunkPos));
	if (pending != pendingChunks.end())
	{
		auto chunk = pending->second.get();
		pendingChunks.erase(pending);
		auto [it, inserted] = chunks.emplace(key(chunkPos), std::move(chunk));
		if (inserted) activateGeneratedBlocks(*it->second, chunkPos);
		return it->second.get();
	}
	auto chunk = std::make_unique<Chunk>();
	rememberStructureVersion(chunkPos);
	generateChunk(*chunk, chunkPos);

	auto [it, inserted] = chunks.emplace(key(chunkPos), std::move(chunk));
	if (inserted) activateGeneratedBlocks(*it->second, chunkPos);
	return it->second.get();
}

void World::activateGeneratedBlocks(Chunk& chunk, m5::ivec5 chunkPos)
{
	const m5::ivec5 corner = chunkPos * m5::ivec5{Chunk::HEIGHT, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE};
	int previousSector = temporalCurrent;
	for (int sector = 0; sector < (temporalEnabled ? 11 : 1); ++sector)
	{
	if (temporalEnabled) selectTemporalSimulation(sector);
	for (auto local : chunk.generatedActiveBlocks)
	{
		auto pos = corner + m5::ivec5(local);
		auto id = getLoadedBlock(pos);
		if (id == ExtendedBlock::SAND) wakeFallingSandAt(pos);
		if (id == ExtendedBlock::ANTI_SAND) wakeAntiSandAt(pos);
		if (!isRedstoneBlock(id)) continue;
		wakeCableAt(pos); wakeCableNeighbors(pos);
		wakeRepeaterAt(pos); wakeRepeaterNeighbors(pos);
		wakeRedirectorAt(pos); wakeRedirectorNeighbors(pos);
		wakeCrystalDoorAt(pos); wakeCrystalDoorNeighbors(pos);
		activateRedstoneTicksNear(pos);
	}
	}
	if (temporalEnabled) selectTemporalSimulation(previousSector);
	chunk.generatedActiveBlocks.clear();
	applyTemporalChunk(chunk, chunkPos);
}

Chunk* World::createEmptyChunk(m5::ivec5 chunkPos)
{
	rememberStructureVersion(chunkPos);
	auto chunk = std::make_unique<Chunk>();
	for (int i = 0; i < Chunk::RENDER_SECTIONS; ++i)
	{
		chunk->shouldUpdateRenderer[i] = true;
	}

	auto [it, inserted] = chunks.emplace(key(chunkPos), std::move(chunk));
	if (inserted) applyTemporalChunk(*it->second, chunkPos);
	return it->second.get();
}

Chunk* World::getChunk(m5::ivec5 chunkPos)
{
	if (!containsPosition(chunkPos)) return nullptr;
	auto it = chunks.find(key(chunkPos));
	if (it != chunks.end())
	{
		return it->second.get();
	}

	if (!canGenerateChunk(chunkPos))
	{
		if (temporalEnabled && temporalAffected.contains(key(chunkPos))) return createEmptyChunk(chunkPos);
		return nullptr;
	}

	return createChunk(chunkPos);
}

void World::requestChunk(m5::ivec5 chunkPos, bool force)
{
	if (!containsPosition(chunkPos)) return;
	auto k = key(chunkPos);
	if (chunks.contains(k) || pendingChunks.contains(k) || pendingChunks.size() >= 24)
	{
		return;
	}
	if (!canGenerateChunk(chunkPos))
	{
		if (force || (temporalEnabled && temporalAffected.contains(k))) createEmptyChunk(chunkPos);
		return;
	}
	rememberStructureVersion(chunkPos);
	// Workers own their generator and result, never the live world's containers.
	pendingChunks.emplace(k, BackgroundWork::generation().addTaskFuture(
		[chunkPos, edge = edgeLength, seed = seed, flat = flat, caves = caves, dimensions = dimensions, structures = structuresFor(chunkPos)]()
		{
			World generator(edge, seed, flat, caves, dimensions);
			generator.structures = structures;
			auto chunk = std::make_unique<Chunk>();
			generator.generateChunk(*chunk, chunkPos);
			return chunk;
		}));
}

bool World::collectGeneratedChunks(size_t maxChunks)
{
	bool changed = false;
	size_t collected = 0;
	for (auto it = pendingChunks.begin(); it != pendingChunks.end();)
	{
		if (collected >= maxChunks) break;
		if (it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			++it;
			continue;
		}
		auto chunk = it->second.get();
		// A loaded save or an edit always wins over a generated result.
		auto [loaded, inserted] = chunks.emplace(it->first, std::move(chunk));
		if (inserted)
		{
			auto& k = loaded->first;
			activateGeneratedBlocks(*loaded->second, {std::get<0>(k), std::get<1>(k), std::get<2>(k), std::get<3>(k), std::get<4>(k)});
			changed = true;
		}
		it = pendingChunks.erase(it);
		++collected;
	}
	return changed;
}

const Chunk* World::findChunk(m5::ivec5 chunkPos) const
{
	auto it = chunks.find(key(chunkPos));
	if (it == chunks.end())
	{
		return nullptr;
	}

	return it->second.get();
}

void World::setChunk(m5::ivec5 chunkPos, const Chunk& chunk)
{
	if (!containsPosition(chunkPos)) return;
	rememberStructureVersion(chunkPos);
	auto dst = std::make_unique<Chunk>();
	dst->blocks = chunk.blocks;
	dst->extendedBlocks = chunk.extendedBlocks;
	for (int i = 0; i < Chunk::RENDER_SECTIONS; ++i)
	{
		dst->shouldUpdateRenderer[i] = true;
		dst->rendererHandleIndices[i] = 0;
		dst->rendererTexIndices[i] = 0;
	}
	chunks[key(chunkPos)] = std::move(dst);
	savedChunks.insert(key(chunkPos));
}

std::vector<std::pair<m5::ivec5, const Chunk*>> World::getSavedChunks() const
{
	std::vector<std::pair<m5::ivec5, const Chunk*>> result;
	result.reserve(savedChunks.size());

	for (const auto& k : savedChunks)
	{
		auto it = chunks.find(k);
		if (it != chunks.end())
		{
			m5::ivec5 pos{ std::get<0>(k), std::get<1>(k), std::get<2>(k), std::get<3>(k), std::get<4>(k) };
			Chunk generated;
			generateChunk(generated, pos);
			if (std::memcmp(&generated.blocks, &it->second->blocks, sizeof(generated.blocks)) != 0 ||
				generated.extendedBlocks != it->second->extendedBlocks)
			{
				result.emplace_back(pos, it->second.get());
			}
		}
	}

	return result;
}

std::vector<World::ChunkDiff> World::getSavedChunkDiffs() const
{
	std::vector<ChunkDiff> result;
	result.reserve(savedChunks.size());

	for (const auto& k : savedChunks)
	{
		auto it = chunks.find(k);
		if (it == chunks.end())
		{
			continue;
		}

		m5::ivec5 pos{ std::get<0>(k), std::get<1>(k), std::get<2>(k), std::get<3>(k), std::get<4>(k) };
		Chunk generated;
		generateChunk(generated, pos);

		ChunkDiff diff;
		diff.pos = pos;

		for (uint8_t b = 0; b < Chunk::SIZE; ++b)
		for (uint8_t c = 0; c < Chunk::SIZE; ++c)
		for (uint8_t d = 0; d < Chunk::SIZE; ++d)
		for (uint8_t e = 0; e < Chunk::SIZE; ++e)
		{
			const auto& curCol = it->second->blocks[b][c][d][e];
			const auto& genCol = generated.blocks[b][c][d][e];

			for (uint8_t i = 0; i < curCol.blockData.size(); ++i)
			{
				uint8_t curByte = curCol.blockData[i];
				uint8_t genByte = genCol.blockData[i];
				if (curByte == genByte && (curByte & 15) != Block::EXTENDED_MARKER && (curByte >> 4) != Block::EXTENDED_MARKER)
				{
					continue;
				}

				for (uint8_t nibble = 0; nibble < 2; ++nibble)
				{
					uint8_t shift = nibble * 4;
					m5::uvec5 localBlock{
						(uint32_t)(i * 2 + nibble),
						(uint32_t)b,
						(uint32_t)c,
						(uint32_t)d,
						(uint32_t)e
					};
					uint16_t curBlock = it->second->getBlockFull(localBlock);
					uint16_t genBlock = generated.getBlockFull(localBlock);
					if (curBlock != genBlock)
					{
						diff.changes.push_back({
							(uint8_t)(i * 2 + nibble),
							b,
							c,
							d,
							e,
							curBlock
						});
					}
				}
			}
		}

		if (!diff.changes.empty())
		{
			result.push_back(std::move(diff));
		}
	}

	return result;
}

Chunk* World::getChunkFromCoords(const glm::vec4& pos)
{
	m5::ivec5 chunkPos{
		0,
		(int)glm::floor(pos.x / (float)Chunk::SIZE),
		(int)glm::floor(pos.y / (float)Chunk::SIZE),
		(int)glm::floor(pos.z / (float)Chunk::SIZE),
		(int)glm::floor(pos.w / (float)Chunk::SIZE)
	};
	return getChunk(chunkPos);
}

const Chunk* World::getChunkFromCoords(const glm::vec4& pos) const
{
	m5::ivec5 chunkPos{
		0,
		(int)glm::floor(pos.x / (float)Chunk::SIZE),
		(int)glm::floor(pos.y / (float)Chunk::SIZE),
		(int)glm::floor(pos.z / (float)Chunk::SIZE),
		(int)glm::floor(pos.w / (float)Chunk::SIZE)
	};
	return findChunk(chunkPos);
}

uint16_t World::getBlock(const m5::ivec5& block)
{
	if (!containsPosition(block)) return Block::AIR;
	if (temporalEnabled && (temporalBase.contains(blockKey(block)) || temporalSectors[temporalCurrent].cells.contains(blockKey(block)))) return temporalCell(block).id;
	m5::ivec5 chunkPos{
		(int)glm::floor(block.a / (float)Chunk::HEIGHT),
		(int)glm::floor(block.b / (float)Chunk::SIZE),
		(int)glm::floor(block.c / (float)Chunk::SIZE),
		(int)glm::floor(block.d / (float)Chunk::SIZE),
		(int)glm::floor(block.e / (float)Chunk::SIZE)
	};
	Chunk* c = getChunk(chunkPos);
	if (!c)
	{
		return Block::AIR;
	}

	m5::uvec5 b
	{
		(uint32_t)(block.a - chunkPos.a * Chunk::HEIGHT),
		(uint32_t)(block.b - chunkPos.b * Chunk::SIZE),
		(uint32_t)(block.c - chunkPos.c * Chunk::SIZE),
		(uint32_t)(block.d - chunkPos.d * Chunk::SIZE),
		(uint32_t)(block.e - chunkPos.e * Chunk::SIZE),
	};

	return c->getBlockFull(b);
}

void World::setBlock(const m5::ivec5& block, uint16_t value)
{
	if (!containsPosition(block)) return;
	m5::ivec5 chunkPos{
		(int)glm::floor(block.a / (float)Chunk::HEIGHT),
		(int)glm::floor(block.b / (float)Chunk::SIZE),
		(int)glm::floor(block.c / (float)Chunk::SIZE),
		(int)glm::floor(block.d / (float)Chunk::SIZE),
		(int)glm::floor(block.e / (float)Chunk::SIZE)
	};
	Chunk* c = const_cast<Chunk*>(findChunk(chunkPos));
	if (!c && canGenerateChunk(chunkPos))
	{
		c = createChunk(chunkPos);
	}
	if (!c && (value != Block::AIR || (temporalEnabled && temporalCell(block).id != Block::AIR)))
	{
		c = createEmptyChunk(chunkPos);
	}
	if (!c)
	{
		return;
	}

	m5::uvec5 b
	{
		(uint32_t)(block.a - chunkPos.a * Chunk::HEIGHT),
		(uint32_t)(block.b - chunkPos.b * Chunk::SIZE),
		(uint32_t)(block.c - chunkPos.c * Chunk::SIZE),
		(uint32_t)(block.d - chunkPos.d * Chunk::SIZE),
		(uint32_t)(block.e - chunkPos.e * Chunk::SIZE),
	};

	uint16_t oldValue = temporalEnabled ? temporalCell(block).id : c->getBlockFull(b);
	if (temporalEnabled) recordTemporal(block, oldValue, value);
	if (!temporalEnabled || temporalCurrent == temporalView) c->setBlock(b, value);
	if (value == ExtendedBlock::STRUCTURE && !structureRegions.contains(blockKey(block)))
		setStructureRegion(block, block + m5::ivec5{1,0,0,0,0}, {8,5,5,1,1});
	else if (oldValue == ExtendedBlock::STRUCTURE && value != oldValue)
		structureRegions.erase(blockKey(block));
	if (oldValue != value)
	{
		bool oldRedstone = isRedstoneBlock(oldValue);
		bool newRedstone = isRedstoneBlock(value);
		if (oldValue == ExtendedBlock::SAND && value != ExtendedBlock::SAND)
		{
			simulation.activeSandBlocks.erase(blockKey(block));
		}
		if (oldValue == ExtendedBlock::ANTI_SAND && value != ExtendedBlock::ANTI_SAND)
		{
			simulation.activeAntiSandBlocks.erase(blockKey(block));
		}
		if (ExtendedBlock::isCable(oldValue) && !ExtendedBlock::isCable(value))
		{
			simulation.activeCableBlocks.erase(blockKey(block));
		}
		if (ExtendedBlock::isCrystalDoor(oldValue) && !ExtendedBlock::isCrystalDoor(value))
		{
			simulation.activeCrystalDoorBlocks.erase(blockKey(block));
		}
		if (ExtendedBlock::isRepeater(oldValue) && !ExtendedBlock::isRepeater(value))
		{
			simulation.activeRepeaterBlocks.erase(blockKey(block));
			simulation.repeaterInputFaces.erase(blockKey(block));
			simulation.repeaterOutputPowers.erase(blockKey(block));
			simulation.repeaterCooldownTicks.erase(blockKey(block));
		}
		if (ExtendedBlock::isRedirector(oldValue) && !ExtendedBlock::isRedirector(value))
		{
			simulation.activeRedirectorBlocks.erase(blockKey(block));
			simulation.redirectorOutputPowers.erase(blockKey(block));
		}
		if (value == ExtendedBlock::SAND)
		{
			wakeFallingSandAt(block);
		}
		if (value == ExtendedBlock::ANTI_SAND)
		{
			wakeAntiSandAt(block);
		}
		if (ExtendedBlock::isCable(value))
		{
			wakeCableAt(block);
		}
		if (ExtendedBlock::isCrystalDoor(value))
		{
			wakeCrystalDoorAt(block);
		}
		if (ExtendedBlock::isRepeater(value))
		{
			wakeRepeaterAt(block);
		}
		if (ExtendedBlock::isRedirector(value))
		{
			wakeRedirectorAt(block);
		}
		if (ExtendedBlock::isCable(oldValue) || ExtendedBlock::isCable(value) ||
			ExtendedBlock::isLever(oldValue) || ExtendedBlock::isLever(value) ||
			ExtendedBlock::isRepeater(oldValue) || ExtendedBlock::isRepeater(value) ||
			ExtendedBlock::isRedirector(oldValue) || ExtendedBlock::isRedirector(value))
		{
			wakeCableNeighbors(block);
			wakeCrystalDoorNeighbors(block);
			wakeRepeaterNeighbors(block);
			wakeRedirectorNeighbors(block);
		}
		if (ExtendedBlock::isCrystalDoor(oldValue) || ExtendedBlock::isCrystalDoor(value))
		{
			wakeCrystalDoorNeighbors(block);
			wakeRepeaterNeighbors(block);
			wakeRedirectorNeighbors(block);
		}
		if (oldRedstone || newRedstone)
		{
			activateRedstoneTicksNear(block);
		}
		if (value == Block::AIR)
		{
			wakeFallingSandAbove(block);
			wakeAntiSandBelow(block);
		}
	}
	savedChunks.insert(key(chunkPos));
}

void World::setBlock(const m5::vec5& block, uint16_t value)
{
	setBlock(m5::ivec5{ m5::floor(block) }, value);
}

bool World::updateFallingSand()
{
	auto destinationReady = [this](const m5::ivec5& block)
	{
		m5::ivec5 pos{
			(int)glm::floor(block.a / (float)Chunk::HEIGHT),
			(int)glm::floor(block.b / (float)Chunk::SIZE),
			(int)glm::floor(block.c / (float)Chunk::SIZE),
			(int)glm::floor(block.d / (float)Chunk::SIZE),
			(int)glm::floor(block.e / (float)Chunk::SIZE) };
		if (findChunk(pos) || !canGenerateChunk(pos)) return true;
		requestChunk(pos);
		return false;
	};
	struct SandMove
	{
		m5::ivec5 from;
		m5::ivec5 to;
		uint16_t value;
	};

	std::vector<SandMove> moves;
	std::set<ChunkKey> reservedTargets;
	std::vector<m5::ivec5> activeBlocks;
	activeBlocks.reserve(simulation.activeSandBlocks.size());
	for (const auto& k : simulation.activeSandBlocks)
	{
		activeBlocks.push_back({
			std::get<0>(k),
			std::get<1>(k),
			std::get<2>(k),
			std::get<3>(k),
			std::get<4>(k)
		});
	}
	simulation.activeSandBlocks.clear();

	for (const m5::ivec5& from : activeBlocks)
	{
		if (getLoadedBlock(from) != ExtendedBlock::SAND)
		{
			continue;
		}

		m5::ivec5 to = from;
		--to.a;
		if (!destinationReady(to))
		{
			wakeFallingSandAt(from);
			continue;
		}
		if (getLoadedBlock(to) == Block::AIR)
		{
			if (!temporalDue(from, 0.5)) { wakeFallingSandAt(from); continue; }
			if (reservedTargets.insert(blockKey(to)).second)
			{
				moves.push_back({ from, to, ExtendedBlock::SAND });
			}
		}
	}

	activeBlocks.clear();
	activeBlocks.reserve(simulation.activeAntiSandBlocks.size());
	for (const auto& k : simulation.activeAntiSandBlocks)
	{
		activeBlocks.push_back({
			std::get<0>(k),
			std::get<1>(k),
			std::get<2>(k),
			std::get<3>(k),
			std::get<4>(k)
		});
	}
	simulation.activeAntiSandBlocks.clear();

	for (const m5::ivec5& from : activeBlocks)
	{
		if (getLoadedBlock(from) != ExtendedBlock::ANTI_SAND)
		{
			continue;
		}

		m5::ivec5 to = from;
		++to.a;
		if (!destinationReady(to))
		{
			wakeAntiSandAt(from);
			continue;
		}
		if (getLoadedBlock(to) == Block::AIR)
		{
			if (!temporalDue(from, 0.5)) { wakeAntiSandAt(from); continue; }
			if (reservedTargets.insert(blockKey(to)).second)
			{
				moves.push_back({ from, to, ExtendedBlock::ANTI_SAND });
			}
		}
	}

	for (const auto& move : moves)
	{
		moveTemporal(move.from, move.to, move.value);
	}

	return !moves.empty();
}

bool World::updateRedstoneCables()
{
	if (!simulation.redstoneTicksActive)
	{
		return false;
	}

	std::vector<m5::ivec5> activeBlocks;
	activeBlocks.reserve(simulation.activeRepeaterBlocks.size());
	for (const auto& k : simulation.activeRepeaterBlocks)
	{
		activeBlocks.push_back({
			std::get<0>(k),
			std::get<1>(k),
			std::get<2>(k),
			std::get<3>(k),
			std::get<4>(k)
		});
	}
	simulation.activeRepeaterBlocks.clear();

	bool changed = false;
	for (const m5::ivec5& block : activeBlocks)
	{
		uint16_t currentBlock = getLoadedBlock(block);
		if (!ExtendedBlock::isRepeater(currentBlock))
		{
			continue;
		}
		if (!temporalDue(block, 0.25)) { wakeRepeaterAt(block); continue; }

		ChunkKey k = blockKey(block);
		uint8_t nextOutput = 0;
		uint8_t inputFace = 255;

		auto cooldownIt = simulation.repeaterCooldownTicks.find(k);
		if (cooldownIt != simulation.repeaterCooldownTicks.end())
		{
			if (cooldownIt->second > 0)
			{
				--cooldownIt->second;
				wakeRepeaterAt(block);
				if (cooldownIt->second == 0)
				{
					simulation.repeaterCooldownTicks.erase(k);
					changed = true;
				}
				continue;
			}
			simulation.repeaterCooldownTicks.erase(k);
		}

		auto lockedFaceIt = simulation.repeaterInputFaces.find(k);
		if (lockedFaceIt != simulation.repeaterInputFaces.end())
		{
			inputFace = lockedFaceIt->second;
			m5::ivec5 source = neighborOnSide(block, inputFace);
			uint16_t sourceBlock = getLoadedBlock(source);
			uint8_t inputPower = redstoneOutputToward(source, sourceBlock, oppositeSide(inputFace));
			if (inputPower > 0)
			{
				nextOutput = ExtendedBlock::repeaterMode(currentBlock);
			}
			else
			{
				inputFace = 255;
				simulation.repeaterInputFaces.erase(k);
				simulation.repeaterCooldownTicks[k] = 20;
				wakeRepeaterAt(block);
			}
		}
		else
		{
			uint8_t inputPower = 0;
			for (int axis = 0; axis < dimensions; ++axis)
			{
				for (int dir : { -1, 1 })
				{
					m5::ivec5 neighbor = block;
					neighbor[axis] += dir;
					uint16_t neighborBlock = getLoadedBlock(neighbor);
					int towardRepeater = sideIndex(axis, -dir);
					uint8_t neighborPower = redstoneOutputToward(neighbor, neighborBlock, towardRepeater);
					if (neighborPower > inputPower)
					{
						inputPower = neighborPower;
						inputFace = (uint8_t)sideIndex(axis, dir);
					}
				}
			}

			if (inputPower > 0)
			{
				nextOutput = ExtendedBlock::repeaterMode(currentBlock);
				simulation.repeaterInputFaces[k] = inputFace;
			}
		}

		uint8_t oldOutput = 0;
		auto outputIt = simulation.repeaterOutputPowers.find(k);
		if (outputIt != simulation.repeaterOutputPowers.end())
		{
			oldOutput = outputIt->second;
		}

		if (nextOutput > 0)
		{
			simulation.repeaterOutputPowers[k] = nextOutput;
		}
		else
		{
			simulation.repeaterOutputPowers.erase(k);
		}

		if (nextOutput != oldOutput)
		{
			wakeCableNeighbors(block);
			wakeRepeaterNeighbors(block);
			wakeCrystalDoorNeighbors(block);
			changed = true;
		}
	}

	activeBlocks.clear();
	activeBlocks.reserve(simulation.activeRedirectorBlocks.size());
	for (const auto& k : simulation.activeRedirectorBlocks)
	{
		activeBlocks.push_back({
			std::get<0>(k),
			std::get<1>(k),
			std::get<2>(k),
			std::get<3>(k),
			std::get<4>(k)
		});
	}
	simulation.activeRedirectorBlocks.clear();

	for (const m5::ivec5& block : activeBlocks)
	{
		uint16_t currentBlock = getLoadedBlock(block);
		if (!ExtendedBlock::isRedirector(currentBlock))
		{
			continue;
		}
		if (!temporalDue(block, 0.25)) { wakeRedirectorAt(block); continue; }

		uint8_t nextPower = 0;
		for (int axis = 0; axis < dimensions; ++axis)
		{
			for (int dir : { -1, 1 })
			{
				int inputSide = sideIndex(axis, dir);
				if (ExtendedBlock::redirectorSideOn(currentBlock, inputSide))
				{
					continue;
				}

				m5::ivec5 neighbor = block;
				neighbor[axis] += dir;
				uint16_t neighborBlock = getLoadedBlock(neighbor);
				int towardRedirector = sideIndex(axis, -dir);
				nextPower = glm::max(nextPower, redstoneOutputToward(neighbor, neighborBlock, towardRedirector));
			}
		}

		ChunkKey k = blockKey(block);
		uint8_t oldPower = 0;
		auto outputIt = simulation.redirectorOutputPowers.find(k);
		if (outputIt != simulation.redirectorOutputPowers.end())
		{
			oldPower = outputIt->second;
		}

		if (nextPower > 0)
		{
			simulation.redirectorOutputPowers[k] = nextPower;
		}
		else
		{
			simulation.redirectorOutputPowers.erase(k);
		}

		if (nextPower != oldPower)
		{
			wakeCableNeighbors(block);
			wakeRepeaterNeighbors(block);
			wakeRedirectorNeighbors(block);
			wakeCrystalDoorNeighbors(block);
			changed = true;
		}
	}

	activeBlocks.clear();
	activeBlocks.reserve(simulation.activeCableBlocks.size());
	for (const auto& k : simulation.activeCableBlocks)
	{
		activeBlocks.push_back({
			std::get<0>(k),
			std::get<1>(k),
			std::get<2>(k),
			std::get<3>(k),
			std::get<4>(k)
		});
	}
	simulation.activeCableBlocks.clear();

	for (const m5::ivec5& block : activeBlocks)
	{
		uint16_t currentBlock = getLoadedBlock(block);
		if (!ExtendedBlock::isCable(currentBlock))
		{
			continue;
		}
		if (!temporalDue(block, 0.25)) { wakeCableAt(block); continue; }

		uint8_t nextPower = 0;
			for (int axis = 0; axis < dimensions; ++axis)
		{
			for (int dir : { -1, 1 })
			{
				m5::ivec5 neighbor = block;
				neighbor[axis] += dir;
				uint16_t neighborBlock = getLoadedBlock(neighbor);
				int towardCable = sideIndex(axis, -dir);
				uint8_t neighborPower = redstoneOutputToward(neighbor, neighborBlock, towardCable);
				if (neighborPower > 0)
				{
					if (neighborBlock == ExtendedBlock::LEVER_ON ||
						ExtendedBlock::isRepeater(neighborBlock) ||
						ExtendedBlock::isRedirector(neighborBlock))
					{
						nextPower = glm::max(nextPower, neighborPower);
					}
					else
					{
						nextPower = glm::max(nextPower, (uint8_t)(neighborPower - 1));
					}
				}
			}
		}

		if (nextPower != ExtendedBlock::cablePower(currentBlock))
		{
			setBlock(block, ExtendedBlock::cableId(nextPower));
			wakeCableNeighbors(block);
			changed = true;
		}
	}

	activeBlocks.clear();
	activeBlocks.reserve(simulation.activeCrystalDoorBlocks.size());
	for (const auto& k : simulation.activeCrystalDoorBlocks)
	{
		activeBlocks.push_back({
			std::get<0>(k),
			std::get<1>(k),
			std::get<2>(k),
			std::get<3>(k),
			std::get<4>(k)
		});
	}
	simulation.activeCrystalDoorBlocks.clear();

	for (const m5::ivec5& block : activeBlocks)
	{
		uint16_t currentBlock = getLoadedBlock(block);
		if (!ExtendedBlock::isCrystalDoor(currentBlock))
		{
			continue;
		}
		if (!temporalDue(block, 0.25)) { wakeCrystalDoorAt(block); continue; }

		m5::ivec5 pairBlock = block;
		++pairBlock.a;
		uint16_t pairValue = getLoadedBlock(pairBlock);
		if (!ExtendedBlock::isCrystalDoor(pairValue))
		{
			pairBlock = block;
			--pairBlock.a;
			pairValue = getLoadedBlock(pairBlock);
		}

		bool powered = isRedstonePowered(block);
		if (ExtendedBlock::isCrystalDoor(pairValue))
		{
			powered = powered || isRedstonePowered(pairBlock);
		}

		uint16_t targetState = powered ? ExtendedBlock::CRYSTAL_DOOR_OPEN : ExtendedBlock::CRYSTAL_DOOR;
		if (currentBlock != targetState)
		{
			setBlock(block, targetState);
			changed = true;
		}
		if (ExtendedBlock::isCrystalDoor(pairValue) && pairValue != targetState &&
			(!temporalEnabled || temporalCell(pairBlock).birth == temporalCell(block).birth))
		{
			setBlock(pairBlock, targetState);
			changed = true;
		}
	}

	return changed;
}

bool World::hasActiveRedstoneTicks() const
{
	return simulation.redstoneTicksActive;
}

void World::stopRedstoneTicks()
{
	simulation.redstoneTicksActive = false;
	simulation.activeCableBlocks.clear();
	simulation.activeCrystalDoorBlocks.clear();
	simulation.activeRepeaterBlocks.clear();
	simulation.activeRedirectorBlocks.clear();
}

World::Collision World::dda(const m5::vec5& pos, const m5::vec5& dir, float maxDist, bool generateMissing)
{
	constexpr float EPS = 1e-6;
	constexpr float INF = 1e30;

	const m5::ivec5 CHUNK_SIZE{ Chunk::HEIGHT, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE };
	const m5::vec5 CHUNK_SIZE_F = CHUNK_SIZE;

	m5::ivec5 step = m5::ivec5(m5::sign(dir));
	m5::vec5 stepF = step;
	m5::vec5 invRd = m5::vec5{ 1.0f } / m5::max(m5::abs(dir), m5::vec5{ EPS });
	m5::vec5 tDelta = invRd;

	m5::vec5 tDeltaC = invRd * CHUNK_SIZE_F;
	m5::vec5 roChunk = pos / CHUNK_SIZE_F;
	m5::ivec5 vC = m5::floor(roChunk);
	m5::vec5 tMaxC = m5::mix(m5::fract(roChunk), m5::vec5{ 1.0f } - m5::fract(roChunk), m5::greaterThan(stepF, m5::vec5{ 0.0f })) * tDeltaC;

	for (int i = 0; i < 5; ++i)
	{
		if (step[i] == 0)
		{
			tDelta[i] = INF;
			tDeltaC[i] = INF;
			tMaxC[i] = INF;
		}
	}

	float t = 0.0;
	int axisC = -1;

	int maxChunkSteps = 4 + (int)glm::ceil(maxDist / (float)Chunk::SIZE) * 4 + 8;

	constexpr auto minAxis5 = [](const m5::vec5& v)
		{
			float m = v.a;
			int axis = 0;

			if (v.b < m) { m = v.b; axis = 1; }
			if (v.c < m) { m = v.c; axis = 2; }
			if (v.d < m) { m = v.d; axis = 3; }
			if (v.e < m) { axis = 4; }

			return axis;
		};

	for (int chunkI = 0; chunkI < maxChunkSteps && t < maxDist; ++chunkI)
	{
		const Chunk* chunk = generateMissing ? getChunk(vC) : findChunk(vC);
		if (!chunk && !generateMissing && canGenerateChunk(vC))
		{
			requestChunk(vC);
			Collision missing{};
			missing.dist = t; missing.pos = pos + dir * t; missing.unloaded = true;
			return missing;
		}
		if (!chunk)
		{
			axisC = minAxis5(tMaxC);
			t = tMaxC[axisC];
			tMaxC[axisC] = tMaxC[axisC] + tDeltaC[axisC];
			vC[axisC] = vC[axisC] + step[axisC];
			continue;
		}

		m5::vec5 l_ro = ((pos + (dir * t)) - (m5::ivec5{ vC } * CHUNK_SIZE_F));
		m5::ivec5 v = m5::clamp(m5::floor(l_ro), m5::vec5{ 0.0f }, CHUNK_SIZE_F - 1.0f);
		m5::vec5 vF = v;
		m5::vec5 tMax = m5::mix(l_ro - vF, m5::vec5{ 1.0f } - (l_ro - vF), m5::greaterThan(stepF, m5::vec5{ 0.0f })) * tDelta;

		for (int i = 0; i < 5; ++i)
		{
			if (step[i] == 0)
			{
				tMax[i] = INF;
			}
		}

		int axis5 = (axisC >= 0) ? axisC : minAxis5(tMax);
		float t5 = 0.0;

		for (int blockI = 0; blockI < 128 && t + t5 < maxDist; ++blockI)
		{
			if (m5::any(m5::lessThan(v, m5::ivec5{ 0 })) || m5::any(m5::greaterThanEqual(v, CHUNK_SIZE))) break;

			uint16_t blockId = chunk->getBlockFull(v);

			if (ExtendedBlock::isSolid(blockId))
			{
				Collision res;
				res.blockId = blockId;
				res.dist = t + t5;
				res.pos = pos + dir * res.dist;
				res.blockPos = (vC * CHUNK_SIZE) + v;
				res.normal = m5::vec5{ 0 };
				res.normal[axis5] = -stepF[axis5];
				res.side = axis5 * 2 + (res.normal[axis5] < 0.0 ? 0 : 1);

				return res;
			}

			axis5 = minAxis5(tMax);
			t5 = tMax[axis5];
			tMax[axis5] = tMax[axis5] + tDelta[axis5];
			v[axis5] = v[axis5] + step[axis5];
		}

		axisC = minAxis5(tMaxC);
		t = tMaxC[axisC];
		tMaxC[axisC] = tMaxC[axisC] + tDeltaC[axisC];
		vC[axisC] = vC[axisC] + step[axisC];
	}

	if (axisC < 0)
	{
		axisC = 0;
	}

	m5::vec5 l_ro = ((pos + (dir * t)) - (m5::ivec5{ vC } * CHUNK_SIZE_F));
	m5::ivec5 v = m5::clamp(m5::floor(l_ro), m5::vec5{ 0.0f }, CHUNK_SIZE_F - 1.0f);

	Collision res;
	res.blockId = Block::AIR;
	res.dist = t;
	res.blockPos = (vC * CHUNK_SIZE) + v;
	res.normal = m5::vec5{ 0 };
	res.normal[axisC] = -stepF[axisC];
	res.side = axisC * 2 + (res.normal[axisC] < 0.0 ? 0 : 1);
	return res;
}

void World::generateChunk(Chunk& chunk, m5::ivec5 chunkPos) const
{
	generateTerrain(chunk, chunkPos);
	applyStructures(chunk, chunkPos);
}

void World::applyStructures(Chunk& chunk, m5::ivec5 chunkPos) const
{
	auto structures = structuresFor(chunkPos);
	if (!structures || structures->empty()) return;
	bool relevant = false;
	for (const auto& rule : *structures)
		relevant |= rule.minHeight - rule.foundationDepth < (chunkPos.a + 1) * (int)Chunk::HEIGHT &&
			rule.maxHeight + rule.structure.size.a >= chunkPos.a * (int)Chunk::HEIGHT;
	if (!relevant) return;

	OpenSimplexNoise noise(seed), biomeNoise(seed ^ 0x9E3779B97F4A7C15ull);
	OpenSimplexNoise regionNoise(seed ^ 0xD1B54A32D192ED03ull);
	OpenSimplexNoise shapeNoise(seed ^ 0xABC98388FB8FAC03ull);
	OpenSimplexNoise detailNoise(seed ^ 0x8CB92BA72F3D8DD7ull);
	std::unordered_map<ChunkKey, float> density;
	std::unordered_map<ChunkKey, std::pair<int, bool>> columns;
	// Query only the columns needed by candidate structures. No neighboring chunks
	// are generated, and the 2-block cave lattice matches generateTerrain exactly.
	auto cave = [&](const m5::ivec5& p)
	{
		m5::ivec5 base;
		for (int i = 0; i < 5; ++i) base[i] = Structures::floorDiv(p[i], 2) * 2;
		auto sample = [&](auto&& self, m5::ivec5 node, int axis) -> float
		{
			if (axis == 5)
			{
				auto k = key(node);
				auto found = density.find(k);
				if (found != density.end()) return found->second;
				float value = caveDensity(regionNoise, shapeNoise, detailNoise,
					glm::dvec4(node.b, node.c, node.d, node.e), node.a);
				density.emplace(k, value);
				return value;
			}
			float first = self(self, node, axis + 1);
			if (p[axis] == node[axis]) return first;
			node[axis] += 2;
			return lerp(first, self(self, node, axis + 1), 0.5f);
		};
		return sample(sample, base, 0);
	};
	auto terrain = [&](const m5::ivec5& p) -> uint16_t
	{
		if (flat)
		{
			if (p.a < 0 || p.a > 8) return Block::AIR;
			return p.a < 4 ? Block::STONE : (p.a < 8 ? Block::DIRT : Block::GRASS);
		}
		if (p.a < 0)
			return caves && cave(p) > 0.0f ? Block::AIR : Block::STONE;
		auto k = key({0, p.b, p.c, p.d, p.e});
		auto found = columns.find(k);
		if (found == columns.end())
		{
			glm::dvec4 terrainPos = glm::dvec4(p.b, p.c, p.d, p.e) * 0.1;
			glm::dvec4 biomePos = glm::dvec4(p.b, p.c, p.d, p.e) * 0.006;
			float n = (noise.Evaluate(terrainPos.x, terrainPos.y, terrainPos.z, terrainPos.w) * 0.5f) + 0.5f;
			int h = glm::clamp((int)glm::round(n * 20.0f + 20.0f), 4, getNaturalTerrainMaxA());
			bool sand = (float)biomeNoise.Evaluate(biomePos.x, biomePos.y, biomePos.z, biomePos.w) > 0.12f;
			found = columns.emplace(k, std::pair{h, sand}).first;
		}
		auto [height, sand] = found->second;
		if (p.a > height || (caves && p.a <= 20 && cave(p) > 0.0f)) return Block::AIR;
		if (sand && p.a > height - 4) return ExtendedBlock::SAND;
		return p.a == height ? Block::GRASS : (p.a > height - 4 ? Block::DIRT : Block::STONE);
	};
	auto biome = [&](const m5::ivec5& p) -> std::string
	{
		if (flat) return "default";
		glm::dvec4 q = glm::dvec4(p.b, p.c, p.d, p.e) * 0.006;
		return (float)biomeNoise.Evaluate(q.x, q.y, q.z, q.w) > 0.12f ? "test" : "default";
	};
	Structures::apply(chunk, chunkPos, seed, *structures, terrain, biome, dimensions);
}

void World::generateTerrain(Chunk& chunk, m5::ivec5 chunkPos) const
{
	constexpr double NOISE_SCALE = 0.1;
	constexpr double BIOME_SCALE = 0.006;
	const int sizeC = dimensions >= 3 ? Chunk::SIZE : 1;
	const int sizeD = dimensions >= 4 ? Chunk::SIZE : 1;
	const int sizeE = dimensions >= 5 ? Chunk::SIZE : 1;
	const int TOTAL_COLUMNS = Chunk::SIZE * sizeC * sizeD * sizeE;

	if (flat)
	{
		if (chunkPos.a != 0)
		{
			return;
		}

		for (int columnIndex = 0; columnIndex < TOTAL_COLUMNS; ++columnIndex)
		{
			int rem = columnIndex;
			int e = rem % sizeE; rem /= sizeE;
			int d = rem % sizeD; rem /= sizeD;
			int c = rem % sizeC; rem /= sizeC;
			int b = rem;
			Chunk::BlockColumn& column = chunk.blocks[b][c][d][e];

			constexpr uint8_t DOUBLE_STONE = Block::STONE | (Block::STONE << 4);
			constexpr uint8_t DOUBLE_DIRT = Block::DIRT | (Block::DIRT << 4);

			std::memset(&column.blockData[0], DOUBLE_STONE, 2);
			std::memset(&column.blockData[2], DOUBLE_DIRT, 2);
			column.blockData[4] = Block::GRASS;
		}
		chunk.shouldUpdateRenderer[0] = true;
		return;
	}

	if (chunkPos.a < 0 && !caves)
	{
		constexpr uint8_t DOUBLE_STONE = Block::STONE | (Block::STONE << 4);
		for (int columnIndex = 0; columnIndex < TOTAL_COLUMNS; ++columnIndex)
		{
			int rem = columnIndex;
			int e = rem % sizeE; rem /= sizeE;
			int d = rem % sizeD; rem /= sizeD;
			int c = rem % sizeC; rem /= sizeC;
			int b = rem;
			std::memset(chunk.blocks[b][c][d][e].blockData.data(), DOUBLE_STONE, chunk.blocks[b][c][d][e].blockData.size());
		}
		chunk.shouldUpdateRenderer[0] = true;
		return;
	}

	OpenSimplexNoise noise(seed);
	OpenSimplexNoise biomeNoise(seed ^ 0x9E3779B97F4A7C15ull);
	OpenSimplexNoise caveRegionNoise(seed ^ 0xD1B54A32D192ED03ull);
	OpenSimplexNoise caveShapeNoise(seed ^ 0xABC98388FB8FAC03ull);
	OpenSimplexNoise caveDetailNoise(seed ^ 0x8CB92BA72F3D8DD7ull);
	glm::dvec4 chunkCorner = glm::dvec4{ chunkPos.b, chunkPos.c, chunkPos.d, chunkPos.e } * (double)Chunk::SIZE;

	std::optional<CaveCache> caveCache;
	if (caves && chunkPos.a * (int)Chunk::HEIGHT <= 20)
	{
		caveCache.emplace();
		for (int ca = 0; ca < CaveCache::A_SIZE; ++ca)
		for (int cb = 0; cb < CaveCache::SIDE_SIZE; ++cb)
		for (int cc = 0; cc < (dimensions >= 3 ? CaveCache::SIDE_SIZE : 1); ++cc)
		for (int cd = 0; cd < (dimensions >= 4 ? CaveCache::SIDE_SIZE : 1); ++cd)
		for (int ce = 0; ce < (dimensions >= 5 ? CaveCache::SIDE_SIZE : 1); ++ce)
		{
			int localA = ca * CaveCache::STEP;
			int worldA = chunkPos.a * (int)Chunk::HEIGHT + localA;
			glm::dvec4 worldPos = chunkCorner + glm::dvec4{
				cb * CaveCache::STEP,
				cc * CaveCache::STEP,
				cd * CaveCache::STEP,
				ce * CaveCache::STEP
			};
			caveCache->set(ca, cb, cc, cd, ce, caveDensity(caveRegionNoise, caveShapeNoise, caveDetailNoise, worldPos, worldA));
		}
	}

	for (int columnIndex = 0; columnIndex < TOTAL_COLUMNS; ++columnIndex)
	{
		int rem = columnIndex;
		int e = rem % sizeE; rem /= sizeE;
		int d = rem % sizeD; rem /= sizeD;
		int c = rem % sizeC; rem /= sizeC;
		int b = rem;
		Chunk::BlockColumn& column = chunk.blocks[b][c][d][e];

		glm::dvec4 worldPos = chunkCorner + glm::dvec4{ b, c, d, e };
		glm::dvec4 terrainPos = worldPos * NOISE_SCALE;
		glm::dvec4 biomePos = worldPos * BIOME_SCALE;

		float noiseVal = (noise.Evaluate(terrainPos.x, terrainPos.y, terrainPos.z, terrainPos.w) * 0.5f) + 0.5f;
		float biomeVal = (float)biomeNoise.Evaluate(biomePos.x, biomePos.y, biomePos.z, biomePos.w);
		bool testBiome = biomeVal > 0.12f;

		int h = glm::round((noiseVal * 20.0f) + 20.0f);
		h = glm::clamp(h, 4, getNaturalTerrainMaxA());

		for (int a = 0; a < Chunk::HEIGHT; ++a)
		{
			int worldA = chunkPos.a * (int)Chunk::HEIGHT + a;
			if (worldA > h)
			{
				continue;
			}

			uint8_t blockType = Block::STONE;

			if (worldA < 0)
			{
				blockType = Block::STONE;
			}
			else
			{
				int surfaceDist = worldA - h;
				if (testBiome && surfaceDist > -4)
				{
					blockType = Block::EXTENDED_MARKER;
				}
				else if (surfaceDist == 0)
				{
					blockType = Block::GRASS;
				}
				else if (surfaceDist > -4)
				{
					blockType = Block::DIRT;
				}
			}

			if (caveCache && sampleCaveCache(*caveCache, a, b, c, d, e) > 0.0f)
			{
				blockType = Block::AIR;
			}

			setPackedBlock(column, a, blockType);
			if (blockType == Block::EXTENDED_MARKER)
			{
				chunk.extendedBlocks[Chunk::blockKey({
					(uint32_t)a,
					(uint32_t)b,
					(uint32_t)c,
					(uint32_t)d,
					(uint32_t)e
				})] = ExtendedBlock::SAND;
			}
		}
	}

	chunk.shouldUpdateRenderer[0] = true;
}

void World::generateChunkDeepstone(Chunk& chunk) const
{
	constexpr uint8_t DOUBLE_STONE = Block::STONE | (Block::STONE << 4);

	for (int b = 0; b < Chunk::SIZE; ++b)
	for (int c = 0; c < Chunk::SIZE; ++c)
	for (int d = 0; d < Chunk::SIZE; ++d)
	for (int e = 0; e < Chunk::SIZE; ++e)
	{
		Chunk::BlockColumn& column = chunk.blocks[b][c][d][e];
		std::memset(column.blockData.data(), DOUBLE_STONE, column.blockData.size());
	}

	chunk.shouldUpdateRenderer[0] = true;
}

void World::generateChunkFlat(Chunk& chunk, int aChunk) const
{
	if (aChunk != 0)
	{
		return;
	}

	for (int b = 0; b < Chunk::SIZE; ++b)
	for (int c = 0; c < Chunk::SIZE; ++c)
	for (int d = 0; d < Chunk::SIZE; ++d)
	for (int e = 0; e < Chunk::SIZE; ++e)
	{
		Chunk::BlockColumn& column = chunk.blocks[b][c][d][e];

		constexpr uint8_t DOUBLE_STONE = Block::STONE | (Block::STONE << 4);
		constexpr uint8_t DOUBLE_DIRT = Block::DIRT | (Block::DIRT << 4);

		std::memset(&column.blockData[0], DOUBLE_STONE, 2);
		std::memset(&column.blockData[2], DOUBLE_DIRT, 2);
		column.blockData[4] = Block::GRASS;
	}

	chunk.shouldUpdateRenderer[0] = true;
}

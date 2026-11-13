#pragma once
#include "Chunk.h"
#include "Structures.h"
#include <memory>
#include <map>
#include <future>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <glm/vec4.hpp>
#include "Math5D.h"
#include "tuple_hash.h"

class World
{
public:
	using ChunkKey = std::tuple<int32_t, int32_t, int32_t, int32_t, int32_t>;
	struct BlockChange
	{
		uint8_t a;
		uint8_t b;
		uint8_t c;
		uint8_t d;
		uint8_t e;
		uint16_t value;
	};
	struct ChunkDiff
	{
		m5::ivec5 pos;
		std::vector<BlockChange> changes;
	};

	World();
	World(uint8_t edgeLength, uint64_t seed = 0, bool flat = false, bool caves = false, int dimensions = 5);
	int getDimensions() const { return dimensions; }
	bool containsPosition(m5::ivec5 pos) const;

	size_t getEdgeLength() const;
	uint64_t getSeed() const;
	bool isFlat() const;
	bool hasCaves() const;

	void setGenerator(uint64_t seed, bool flat, bool caves);
	void setStructures(std::shared_ptr<const Structures::Catalog> catalog);
	void loadStructureHistory(const nlohmann::json& json);
	nlohmann::json saveStructureHistory() const;
	void useSavedStructureBaseline(m5::ivec5 pos);
	void setStructureRegion(m5::ivec5 owner, m5::ivec5 min, m5::ivec5 size);
	void updateStructureRegions();

	size_t getLoadedChunkCount() const;
	int getNaturalTerrainMaxA() const;
	int getNaturalTerrainMaxChunkA() const;
	bool canGenerateChunk(m5::ivec5 chunkPos) const;

	Chunk* getChunk(m5::ivec5 chunkPos);
	void requestChunk(m5::ivec5 chunkPos, bool force = false);
	bool collectGeneratedChunks(size_t maxChunks = SIZE_MAX);
	const Chunk* findChunk(m5::ivec5 chunkPos) const;
	void setChunk(m5::ivec5 chunkPos, const Chunk& chunk);
	std::vector<std::pair<m5::ivec5, const Chunk*>> getSavedChunks() const;
	std::vector<ChunkDiff> getSavedChunkDiffs() const;

	Chunk* getChunkFromCoords(const glm::vec4& pos);
	const Chunk* getChunkFromCoords(const glm::vec4& pos) const;
	Chunk* getChunkFromCoords(float b, float c, float d, float e) { return getChunkFromCoords({ b, c, d, e }); }
	const Chunk* getChunkFromCoords(float b, float c, float d, float e) const { return getChunkFromCoords({ b, c, d, e }); }

	uint16_t getBlock(const m5::ivec5& block);
	void setBlock(const m5::ivec5& block, uint16_t value);
	void setBlock(const m5::vec5& block, uint16_t value);
	bool updateFallingSand();
	bool updateRedstoneCables();
	bool hasActiveRedstoneTicks() const;
	void stopRedstoneTicks();
	void enableTemporal(const std::filesystem::path& file);
	void saveTemporal(const std::filesystem::path& file) const;
	bool hasTemporal() const { return temporalEnabled; }
	int temporalPercent() const { return temporalView * 10; }
	bool changeTemporalSector(int delta);
	bool advanceTemporal(double dt);
	void editTemporalBlock(m5::ivec5 pos, uint16_t value, bool placement = false);
	static double temporalRate(int birthPercent, int sectorPercent);

	m5::uvec5 getSize() const
	{
		size_t size = edgeLength * Chunk::SIZE;
		return m5::uvec5
		{
			Chunk::HEIGHT, size, dimensions >= 3 ? size : 1, dimensions >= 4 ? size : 1, dimensions >= 5 ? size : 1
		};
	}

	struct Collision
	{
		uint16_t blockId;
		float dist;
		m5::vec5 pos;
		// signed in case the block is at the world edge
		m5::ivec5 blockPos;
		m5::vec5 normal;
		int side;
		bool unloaded = false;
	};
	Collision dda(const m5::vec5& pos, const m5::vec5& dir, float maxDist, bool generateMissing = true);

private:

	uint8_t edgeLength;
	int dimensions = 5;
	uint64_t seed = 0;
	bool flat = false;
	bool caves = false;
	std::shared_ptr<const Structures::Catalog> structures;
	std::vector<std::shared_ptr<const Structures::Catalog>> structureHistory;
	std::unordered_map<ChunkKey, size_t> chunkStructureVersions;
	size_t currentStructureVersion = 0;
	std::shared_ptr<const Structures::Catalog> structuresFor(m5::ivec5 pos) const;
	void rememberStructureVersion(m5::ivec5 pos);
	struct StructureRegion
	{
		m5::ivec5 min, max, cursor;
		bool ready = false;
	};
	std::map<ChunkKey, StructureRegion> structureRegions;
	std::unordered_map<ChunkKey, std::unique_ptr<Chunk>> chunks;
	std::unordered_map<ChunkKey, std::future<std::unique_ptr<Chunk>>> pendingChunks;
	std::set<ChunkKey> savedChunks;
	struct Simulation {
		std::set<ChunkKey> activeSandBlocks;
		std::set<ChunkKey> activeAntiSandBlocks;
		std::set<ChunkKey> activeCableBlocks;
		std::set<ChunkKey> activeCrystalDoorBlocks;
		std::set<ChunkKey> activeRepeaterBlocks;
		std::set<ChunkKey> activeRedirectorBlocks;
		std::unordered_map<ChunkKey, uint8_t> repeaterInputFaces;
		std::unordered_map<ChunkKey, uint8_t> repeaterOutputPowers;
		std::unordered_map<ChunkKey, uint8_t> repeaterCooldownTicks;
		std::unordered_map<ChunkKey, uint8_t> redirectorOutputPowers;
		bool redstoneTicksActive = false;
		std::map<ChunkKey, double> clocks;
		double idle = 0;
	};
	Simulation simulation;
	struct TemporalCell { uint16_t id = 0; uint64_t identity = 0; uint8_t birth = 50; };
	struct TemporalSector {
		std::map<ChunkKey, TemporalCell> cells;
		std::map<uint64_t, ChunkKey> locations;
		Simulation simulation;
	};
	std::array<TemporalSector, 11> temporalSectors;
	std::map<ChunkKey, TemporalCell> temporalBase;
	std::map<ChunkKey, std::set<ChunkKey>> temporalAffected;
	bool temporalEnabled = false;
	int temporalView = 5, temporalCurrent = 5;
	uint64_t temporalNextIdentity = 1;
	double temporalAccumulator = 0, temporalStep = 0;
	void selectTemporalSimulation(int sector);
	TemporalCell temporalCell(m5::ivec5 pos) const;
	TemporalCell identifyTemporal(m5::ivec5 pos);
	void recordTemporal(m5::ivec5 pos, uint16_t oldValue, uint16_t value);
	bool temporalDue(m5::ivec5 pos, double period);
	void moveTemporal(m5::ivec5 from, m5::ivec5 to, uint16_t value);
	void applyTemporalChunk(Chunk& chunk, m5::ivec5 pos);
	void refreshTemporalView();

	ChunkKey key(m5::ivec5 chunkPos) const;
	ChunkKey blockKey(m5::ivec5 blockPos) const;
	Chunk* createChunk(m5::ivec5 chunkPos);
	Chunk* createEmptyChunk(m5::ivec5 chunkPos);
	void wakeFallingSandAt(m5::ivec5 blockPos);
	void wakeFallingSandAbove(m5::ivec5 blockPos);
	void wakeAntiSandAt(m5::ivec5 blockPos);
	void wakeAntiSandBelow(m5::ivec5 blockPos);
	void wakeCableAt(m5::ivec5 blockPos);
	void wakeCableNeighbors(m5::ivec5 blockPos);
	void wakeCrystalDoorAt(m5::ivec5 blockPos);
	void wakeCrystalDoorNeighbors(m5::ivec5 blockPos);
	void wakeRepeaterAt(m5::ivec5 blockPos);
	void wakeRepeaterNeighbors(m5::ivec5 blockPos);
	void wakeRedirectorAt(m5::ivec5 blockPos);
	void wakeRedirectorNeighbors(m5::ivec5 blockPos);
	bool isRedstonePowered(m5::ivec5 blockPos);
	uint8_t redstoneOutputToward(m5::ivec5 sourcePos, uint16_t sourceBlock, int towardSide);
	uint16_t getLoadedBlock(const m5::ivec5& block) const;
	bool isBlockLoaded(const m5::ivec5& block) const;
	bool isRedstoneBlock(uint16_t id) const;
	bool hasAdjacentLoadedRedstone(m5::ivec5 blockPos) const;
	void activateRedstoneTicksNear(m5::ivec5 blockPos);
	void generateChunk(Chunk& chunk, m5::ivec5 chunkPos) const;
	void generateTerrain(Chunk& chunk, m5::ivec5 chunkPos) const;
	void applyStructures(Chunk& chunk, m5::ivec5 chunkPos) const;
	void activateGeneratedBlocks(Chunk& chunk, m5::ivec5 chunkPos);
	void generateChunkFlat(Chunk& chunk, int aChunk) const;
	void generateChunkDeepstone(Chunk& chunk) const;
};

#pragma once
#include "Chunk.h"
#include <memory>
#include <glm/vec4.hpp>
#include "Math5D.h"

class World
{
public:
	World();
	World(uint8_t edgeLength);

	size_t getEdgeLength() const;

	size_t getDataSize() const;

	// returns a pointer to the 4d array of chunks
	Chunk* getChunks();
	const Chunk* getChunks() const;

	Chunk* getChunk(glm::u8vec4 chunkPos);
	const Chunk* getChunk(glm::u8vec4 chunkPos) const;

	Chunk* getChunkFromCoords(const glm::vec4& pos);
	const Chunk* getChunkFromCoords(const glm::vec4& pos) const;
	Chunk* getChunkFromCoords(float b, float c, float d, float e) { return getChunkFromCoords({ b, c, d, e }); }
	const Chunk* getChunkFromCoords(float b, float c, float d, float e) const { return getChunkFromCoords({ b, c, d, e }); }

	uint8_t getBlock(const m5::uvec5& block) const;
	void setBlock(const m5::uvec5& block, uint8_t value);

	m5::uvec5 getSize() const
	{
		size_t size = edgeLength * Chunk::SIZE;
		return m5::uvec5
		{
			Chunk::HEIGHT, size, size, size, size
		};
	}

	struct Collision
	{
		uint8_t blockId;
		float dist;
		m5::vec5 pos;
		// signed in case the block is at the world edge
		m5::ivec5 blockPos;
		m5::vec5 normal;
		int side;
	};
	Collision dda(const m5::vec5& pos, const m5::vec5& dir, float maxDist);

private:

	uint8_t edgeLength;
	std::unique_ptr<Chunk[]> chunks;
};
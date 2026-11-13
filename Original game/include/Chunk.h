#pragma once
#include "Block.h"
#include <array>
#include <cstdint>
#include <cassert>
#include <atomic>
#include "Math5D.h"

class Chunk
{
public:
	inline static constexpr size_t SIZE = 8;
	inline static constexpr size_t HEIGHT = 128;
	inline static const m5::u8vec5 CHUNK_SIZE{ HEIGHT, SIZE, SIZE, SIZE, SIZE };

	struct BlockColumn
	{
		// maximum 4-bit value, 15
		inline static constexpr uint8_t BLOCK_ID_MAX = 0b00001111;

		std::array<uint8_t, HEIGHT / 2> blockData{ 0 };

		uint8_t getBlock(size_t index) const;
		void setBlock(size_t index, uint8_t value);
	};

	using BlockArr = std::array<std::array<std::array<std::array<BlockColumn, SIZE>, SIZE>, SIZE>, SIZE>;
	BlockArr blocks{ {} };

	std::atomic<bool> shouldUpdateRenderer[4]{ false, false, false, false };
	size_t rendererHandleIndices[4]{ 0 };
	size_t rendererTexIndices[4]{ 0 };

	Chunk() {}
	~Chunk() {}

	uint8_t getBlock(const m5::uvec5& block) const;
	void setBlock(const m5::uvec5& block, uint8_t value);
};
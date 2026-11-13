#include "Chunk.h"

uint8_t Chunk::BlockColumn::getBlock(size_t index) const
{
	assert(index < Chunk::HEIGHT);

	int shift = 4 * (index & 1);

	return (blockData[index >> 1] >> shift) & BLOCK_ID_MAX;
}

void Chunk::BlockColumn::setBlock(size_t index, uint8_t value)
{
	assert(index < Chunk::HEIGHT);
	assert(value <= BLOCK_ID_MAX);

	int shift = 4 * (index & 1);

	uint8_t& dst = blockData[index >> 1];
	// clear the destination bits
	dst &= (uint8_t)0b11110000 >> shift;
	// apply the block-value bits
	dst |= value << shift;
}

uint8_t Chunk::getBlock(const m5::uvec5& block) const
{
	assert(m5::max(block, (CHUNK_SIZE - 1)) == (CHUNK_SIZE - 1));

	return blocks[block.b][block.c][block.d][block.e].getBlock(block.a);
}

void Chunk::setBlock(const m5::uvec5& block, uint8_t value)
{
	assert(m5::max(block, (CHUNK_SIZE - 1)) == (CHUNK_SIZE - 1));

	blocks[block.b][block.c][block.d][block.e].setBlock(block.a, value);
	shouldUpdateRenderer[block.a / 32] = true;
}
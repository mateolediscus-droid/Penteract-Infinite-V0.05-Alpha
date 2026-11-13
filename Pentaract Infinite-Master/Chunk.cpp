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
	dst &= 0b11110000ui8 >> shift;
	// apply the block-value bits
	dst |= value << shift;
}

uint8_t Chunk::getBlock(const m5::uvec5& block) const
{
	assert(m5::max(block, (CHUNK_SIZE - 1)) == (CHUNK_SIZE - 1));

	return blocks[block.b][block.c][block.d][block.e].getBlock(block.a);
}

uint16_t Chunk::getBlockFull(const m5::uvec5& block) const
{
	assert(m5::max(block, (CHUNK_SIZE - 1)) == (CHUNK_SIZE - 1));

	uint8_t base = getBlock(block);
	if (base == Block::EXTENDED_RENDER_DEEPSTONE)
	{
		return ExtendedBlock::DEEPSTONE;
	}
	if (base == Block::EXTENDED_RENDER_LEVER_OFF)
	{
		return ExtendedBlock::LEVER_OFF;
	}
	if (base == Block::EXTENDED_RENDER_LEVER_ON)
	{
		return ExtendedBlock::LEVER_ON;
	}
	if (base != Block::EXTENDED_MARKER)
	{
		return base;
	}

	auto it = extendedBlocks.find(blockKey(block));
	return it == extendedBlocks.end() ? Block::AIR : it->second;
}

void Chunk::setBlock(const m5::uvec5& block, uint16_t value)
{
	assert(m5::max(block, (CHUNK_SIZE - 1)) == (CHUNK_SIZE - 1));

	if (value >= ExtendedBlock::FIRST)
	{
		blocks[block.b][block.c][block.d][block.e].setBlock(block.a, Block::EXTENDED_MARKER);
		extendedBlocks[blockKey(block)] = value;
	}
	else
	{
		assert(value < Block::EXTENDED_MARKER);
		blocks[block.b][block.c][block.d][block.e].setBlock(block.a, (uint8_t)value);
		extendedBlocks.erase(blockKey(block));
	}

	shouldUpdateRenderer[block.a / 32] = true;
}

uint32_t Chunk::blockKey(const m5::uvec5& block)
{
	return (uint32_t)block.a |
		((uint32_t)block.b << 7) |
		((uint32_t)block.c << 10) |
		((uint32_t)block.d << 13) |
		((uint32_t)block.e << 16);
}

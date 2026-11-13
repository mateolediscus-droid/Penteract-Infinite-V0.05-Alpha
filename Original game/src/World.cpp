#include "World.h"
#include "Math5D.h"
#include <glm/common.hpp>

World::World() : edgeLength{ 0 }
{
}

World::World(uint8_t edgeLength) :
	edgeLength{ edgeLength },
	chunks{ std::make_unique<Chunk[]>(edgeLength * edgeLength * edgeLength * edgeLength) }
{
	assert(edgeLength > 0);
}

size_t World::getEdgeLength() const
{
	return edgeLength;
}

size_t World::getDataSize() const
{
	return sizeof(Chunk) * edgeLength * edgeLength * edgeLength * edgeLength;
}

Chunk* World::getChunks()
{
	return chunks.get();
}

const Chunk* World::getChunks() const
{
	return chunks.get();
}

Chunk* World::getChunk(glm::u8vec4 chunkPos)
{
	if (glm::max(chunkPos, glm::u8vec4{ (uint8_t)(edgeLength - 1) }) != glm::u8vec4{ (uint8_t)(edgeLength - 1) })
	{
		return nullptr;
	}

	size_t index =
		(size_t)chunkPos.x * (edgeLength * edgeLength * edgeLength) +
		(size_t)chunkPos.y * (edgeLength * edgeLength) +
		(size_t)chunkPos.z * (edgeLength) +
		(size_t)chunkPos.w;

	return &chunks[index];
}

const Chunk* World::getChunk(glm::u8vec4 chunkPos) const
{
	if (glm::max(chunkPos, glm::u8vec4{ (uint8_t)(edgeLength - 1) }) != glm::u8vec4{ (uint8_t)(edgeLength - 1) })
	{
		return nullptr;
	}

	size_t index =
		(size_t)chunkPos.x * (edgeLength * edgeLength * edgeLength) +
		(size_t)chunkPos.y * (edgeLength * edgeLength) +
		(size_t)chunkPos.z * (edgeLength)+
		(size_t)chunkPos.w;

	return &chunks[index];
}

Chunk* World::getChunkFromCoords(const glm::vec4& pos)
{
	glm::u8vec4 chunkPos = glm::floor(pos / (float)Chunk::SIZE);
	return getChunk(chunkPos);
}

const Chunk* World::getChunkFromCoords(const glm::vec4& pos) const
{
	glm::u8vec4 chunkPos = glm::floor(pos / (float)Chunk::SIZE);
	return getChunk(chunkPos);
}

uint8_t World::getBlock(const m5::uvec5& block) const
{
	if (m5::max(block, (getSize() - 1)) != (getSize() - 1))
	{
		return 0;
	}

	if (block.a >= Chunk::HEIGHT)
	{
		return 0;
	}

	const Chunk* c = getChunkFromCoords(block.b, block.c, block.d, block.e);
	if (c)
	{
		// coords must be floats
		m5::vec5 coords = block;

		m5::uvec5 b
		{
			coords.a,
			coords.b - (glm::floor(coords.b / Chunk::SIZE) * Chunk::SIZE),
			coords.c - (glm::floor(coords.c / Chunk::SIZE) * Chunk::SIZE),
			coords.d - (glm::floor(coords.d / Chunk::SIZE) * Chunk::SIZE),
			coords.e - (glm::floor(coords.e / Chunk::SIZE) * Chunk::SIZE),
		};

		return c->getBlock(b);
	}

	return Block::AIR;
}

void World::setBlock(const m5::uvec5& block, uint8_t value)
{
	if (m5::max(block, (getSize() - 1)) != (getSize() - 1))
	{
		return;
	}

	if (block.a >= Chunk::HEIGHT)
	{
		return;
	}

	Chunk* c = getChunkFromCoords(block.b, block.c, block.d, block.e);
	if (c)
	{
		// coords must be floats
		m5::vec5 coords = block;

		m5::uvec5 b
		{
			coords.a,
			coords.b - (glm::floor(coords.b / Chunk::SIZE) * Chunk::SIZE),
			coords.c - (glm::floor(coords.c / Chunk::SIZE) * Chunk::SIZE),
			coords.d - (glm::floor(coords.d / Chunk::SIZE) * Chunk::SIZE),
			coords.e - (glm::floor(coords.e / Chunk::SIZE) * Chunk::SIZE),
		};

		c->setBlock(b, value);
	}
}

World::Collision World::dda(const m5::vec5& pos, const m5::vec5& dir, float maxDist)
{
	constexpr float EPS = 1e-6;
	constexpr float INF = 1e30;

	const m5::ivec5 CHUNK_SIZE{ Chunk::HEIGHT, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE };
	const m5::vec5 CHUNK_SIZE_F = CHUNK_SIZE;

	m5::ivec5 step = m5::ivec5(m5::sign(dir));
	m5::vec5 stepF = step;
	m5::vec5 invRd = m5::vec5{ 1.0f } / m5::max(m5::abs(dir), m5::vec5{ EPS });
	m5::vec5 tDelta = invRd;

	// DDA into chunks
	m5::vec5 tDeltaC = invRd * CHUNK_SIZE_F;
	m5::vec5 roChunk = pos / CHUNK_SIZE_F;
	m5::ivec5 vC = m5::floor(roChunk);
	m5::vec5 tMaxC = m5::mix(m5::fract(roChunk), m5::vec5{ 1.0f } - m5::fract(roChunk), m5::greaterThan(stepF, m5::vec5{ 0.0f })) * tDeltaC;

	glm::vec4 chunksMinBound{ 0.0f };
	glm::ivec4 chunksSize{ edgeLength };

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
	bool checkBackSide = false;

	int maxChunkSteps = 4 + edgeLength + edgeLength + edgeLength + edgeLength + 8;

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
		// early bounds check
		glm::ivec4 rel4 = glm::vec4{ vC.b, vC.c, vC.d, vC.e } - chunksMinBound;
		if (glm::any(glm::lessThan(rel4, glm::ivec4(0))) || glm::any(glm::greaterThanEqual(rel4, chunksSize)) ||
			vC.a < 0 || vC.a >= 4)
		{
			axisC = minAxis5(tMaxC);
			t = tMaxC[axisC];
			tMaxC[axisC] = tMaxC[axisC] + tDeltaC[axisC];
			vC[axisC] = vC[axisC] + step[axisC];
			continue;
		}

		Chunk* chunk = getChunk({ vC.b, vC.c, vC.d, vC.e });

		// DDA into blocks
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
		checkBackSide = false;

		for (int blockI = 0; blockI < 128 && t + t5 < maxDist; ++blockI)
		{
			if (m5::any(m5::lessThan(v, m5::ivec5{ 0 })) || m5::any(m5::greaterThanEqual(v, CHUNK_SIZE))) break;

			uint8_t blockId = chunk->getBlock(v);

			if (blockId != Block::AIR)
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

	m5::vec5 l_ro = ((pos + (dir * t)) - (m5::ivec5{ vC } *CHUNK_SIZE_F));
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

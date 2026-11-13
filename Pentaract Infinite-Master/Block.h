#pragma once
#include <cstdint>
#include <array>
#include <string_view>
#include <vector>

namespace Block
{
	inline constexpr uint8_t EXTENDED_MARKER = 15;
	inline constexpr uint8_t EXTENDED_RENDER_DEEPSTONE = 14;
	inline constexpr uint8_t EXTENDED_RENDER_LEVER_ON = 13;
	inline constexpr uint8_t EXTENDED_RENDER_LEVER_OFF = 12;

	enum ID : uint8_t
	{
		AIR,
		GRASS,
		DIRT,
		STONE,
		WOOD,
		GLASS,
		BRICKS,
		COUNT
	};

	struct Data
	{
		int textures[10]{ 0 };
		bool solid = true;
	};

	inline std::array<Data, COUNT> blocks
	{
		// AIR
		Data{{
				0, 0, // -A, +A
				0, 0, // -B, +B
				0, 0, // -C, +C
				0, 0, // -D, +D
				0, 0, // -E, +E
			},
			false
		},
		// GRASS
		Data{{
				0, 2, // -A, +A
				1, 1, // -B, +B
				1, 1, // -C, +C
				1, 1, // -D, +D
				1, 1, // -E, +E
			},
			true
		},
		// DIRT
		Data{{
				0, 0, // -A, +A
				0, 0, // -B, +B
				0, 0, // -C, +C
				0, 0, // -D, +D
				0, 0, // -E, +E
			},
			true
		},
		// STONE
		Data{{
				3, 3, // -A, +A
				3, 3, // -B, +B
				3, 3, // -C, +C
				3, 3, // -D, +D
				3, 3, // -E, +E
			},
			true
		},
		// WOOD
		Data{{
				5, 5, // -A, +A
				4, 4, // -B, +B
				4, 4, // -C, +C
				4, 4, // -D, +D
				4, 4, // -E, +E
			},
			true
		},
		// GLASS
		Data{{
				6, 6, // -A, +A
				6, 6, // -B, +B
				6, 6, // -C, +C
				6, 6, // -D, +D
				6, 6, // -E, +E
			},
			true
		},
		// BRICKS
		Data{{
				7, 7, // -A, +A
				7, 7, // -B, +B
				7, 7, // -C, +C
				7, 7, // -D, +D
				7, 7, // -E, +E
			},
			true
		},
	};

	inline std::array<std::string_view, COUNT> names
	{
		"Air",
		"Grass",
		"Dirt",
		"Stone",
		"Wood",
		"Glass",
		"Bricks",
	};
}

namespace ExtendedBlock
{
	enum ID : uint16_t
	{
		FIRST = 16,
		SAND = FIRST,
		STONE_COPY = SAND,
		DEEPSTONE,
		ANTI_SAND,
		LEVER_OFF,
		LEVER_ON,
		CABLE_0,
		CABLE_1,
		CABLE_2,
		CABLE_3,
		CABLE_4,
		CABLE_5,
		CABLE_6,
		CABLE_7,
		CABLE_8,
		CABLE_9,
		CABLE_10,
		CABLE_11,
		CABLE_12,
		CABLE_13,
		CABLE_14,
		CABLE_15,
		CRYSTAL_DOOR,
		CRYSTAL_DOOR_OPEN,
		REPEATER_0,
		REPEATER_1,
		REPEATER_2,
		REPEATER_3,
		REPEATER_4,
		REPEATER_5,
		REPEATER_6,
		REPEATER_7,
		REPEATER_8,
		REPEATER_9,
		REPEATER_10,
		REPEATER_11,
		REPEATER_12,
		REPEATER_13,
		REPEATER_14,
		REPEATER_15,
		REDIRECTOR_0,
		REDIRECTOR_COUNT = 1024,
		STRUCTURE = REDIRECTOR_0 + REDIRECTOR_COUNT,
		LEAF,
		WATER,
		COUNT
	};

	struct Data
	{
		int textures[10]{ 0 };
		bool solid = true;
		uint8_t iconBlock = Block::STONE;
		std::string_view iconTextureName{};
	};

	inline Data uniformData(int texture, uint8_t iconBlock, std::string_view iconTextureName)
	{
		Data data{};
		for (int& faceTexture : data.textures)
		{
			faceTexture = texture;
		}
		data.solid = true;
		data.iconBlock = iconBlock;
		data.iconTextureName = iconTextureName;
		return data;
	}

	inline Data redirectorData(uint16_t mask)
	{
		Data data = uniformData(48, Block::EXTENDED_MARKER, "extended.redirectorIcon");
		for (int side = 0; side < 10; ++side)
		{
			if (mask & (1u << side))
			{
				data.textures[side] = 49;
			}
		}
		return data;
	}

	inline std::vector<Data> blocks = []()
	{
		std::vector<Data> data(COUNT - FIRST);

		data[SAND - FIRST] = Data{{
				9, 9, // -A, +A
				9, 9, // -B, +B
				9, 9, // -C, +C
				9, 9, // -D, +D
				9, 9, // -E, +E
			},
			true,
			Block::EXTENDED_MARKER,
			""
		};
		data[DEEPSTONE - FIRST] = Data{{
				10, 10, // -A, +A
				10, 10, // -B, +B
				10, 10, // -C, +C
				10, 10, // -D, +D
				10, 10, // -E, +E
			},
			true,
			Block::STONE,
			"extended.deepstoneIcon"
		};
		data[ANTI_SAND - FIRST] = Data{{
				9, 9, // -A, +A
				9, 9, // -B, +B
				9, 9, // -C, +C
				9, 9, // -D, +D
				9, 9, // -E, +E
			},
			true,
			Block::EXTENDED_MARKER,
			"extended.antiSandIcon"
		};
		data[LEVER_OFF - FIRST] = Data{{
				12, 12, // -A, +A
				12, 12, // -B, +B
				12, 12, // -C, +C
				12, 12, // -D, +D
				12, 12, // -E, +E
			},
			true,
			Block::EXTENDED_RENDER_LEVER_OFF,
			"extended.leverIcon"
		};
		data[LEVER_ON - FIRST] = Data{{
				13, 13, // -A, +A
				13, 13, // -B, +B
				13, 13, // -C, +C
				13, 13, // -D, +D
				13, 13, // -E, +E
			},
			true,
			Block::EXTENDED_RENDER_LEVER_ON,
			"extended.leverIcon"
		};

		for (int i = 0; i < 16; ++i)
		{
			data[CABLE_0 - FIRST + i] = uniformData(14 + i, Block::EXTENDED_MARKER, "extended.cableIcon");
		}

		data[CRYSTAL_DOOR - FIRST] = uniformData(30, Block::EXTENDED_MARKER, "extended.crystalDoorIcon");
		data[CRYSTAL_DOOR_OPEN - FIRST] = Data{{
				31, 31, // -A, +A
				31, 31, // -B, +B
				31, 31, // -C, +C
				31, 31, // -D, +D
				31, 31, // -E, +E
			},
			false,
			Block::EXTENDED_MARKER,
			"extended.crystalDoorIcon"
		};

		for (int i = 0; i < 16; ++i)
		{
			data[REPEATER_0 - FIRST + i] = uniformData(32 + i, Block::EXTENDED_MARKER, "extended.repeaterIcon");
		}

		for (int mask = 0; mask < REDIRECTOR_COUNT; ++mask)
		{
			data[REDIRECTOR_0 - FIRST + mask] = redirectorData((uint16_t)mask);
		}

		data[STRUCTURE - FIRST] = uniformData(7, Block::BRICKS, "extended.structureIcon");
		data[LEAF - FIRST] = uniformData(50, Block::GRASS, "extended.leafIcon");
		for (int side = 0; side < 10; ++side) data[LEAF - FIRST].textures[side] = 50 + side;
		data[WATER - FIRST] = uniformData(8, Block::GLASS, "");
		data[WATER - FIRST].solid = false;
		return data;
	}();

	inline bool isExtended(uint16_t id)
	{
		return id >= FIRST && id < COUNT;
	}

	inline const Data& data(uint16_t id)
	{
		return blocks[id - FIRST];
	}

	inline std::string_view name(uint16_t id)
	{
		if (id == SAND) return "Sand";
		if (id == DEEPSTONE) return "Deepstone";
		if (id == ANTI_SAND) return "Anti Sand";
		if (id == LEVER_OFF) return "Lever Off";
		if (id == LEVER_ON) return "Lever On";
		if (id >= CABLE_0 && id <= CABLE_15) return "Wire";
		if (id == CRYSTAL_DOOR || id == CRYSTAL_DOOR_OPEN) return "Crystal Door";
		if (id >= REPEATER_0 && id <= REPEATER_15) return "Repeater";
		if (id >= REDIRECTOR_0 && id < STRUCTURE) return "Redirector";
		if (id == STRUCTURE) return "Structure Block";
		if (id == LEAF) return "Leaf";
		if (id == WATER) return "Water";
		return "Unknown";
	}

	inline uint8_t renderId(uint16_t id)
	{
		switch (id)
		{
		case DEEPSTONE:
			return Block::EXTENDED_RENDER_DEEPSTONE;
		case LEVER_OFF:
			return Block::EXTENDED_RENDER_LEVER_OFF;
		case LEVER_ON:
			return Block::EXTENDED_RENDER_LEVER_ON;
		case ANTI_SAND:
		case SAND:
		default:
			return Block::EXTENDED_MARKER;
		}
	}

	inline bool isLever(uint16_t id)
	{
		return id == LEVER_OFF || id == LEVER_ON;
	}

	inline bool isCable(uint16_t id)
	{
		return id >= CABLE_0 && id <= CABLE_15;
	}

	inline bool isCrystalDoor(uint16_t id)
	{
		return id == CRYSTAL_DOOR || id == CRYSTAL_DOOR_OPEN;
	}

	inline bool isRepeater(uint16_t id)
	{
		return id >= REPEATER_0 && id <= REPEATER_15;
	}

	inline bool isRedirector(uint16_t id)
	{
		return id >= REDIRECTOR_0 && id < STRUCTURE;
	}

	inline bool isRenderable(uint16_t id)
	{
		return id != Block::AIR && (isExtended(id) || id < Block::COUNT);
	}

	inline bool isSolid(uint16_t id)
	{
		if (id == Block::AIR || id == CRYSTAL_DOOR_OPEN)
		{
			return false;
		}
		if (isExtended(id))
		{
			return data(id).solid;
		}
		return id < Block::COUNT && Block::blocks[id].solid;
	}

	inline uint8_t cablePower(uint16_t id)
	{
		return isCable(id) ? (uint8_t)(id - CABLE_0) : 0;
	}

	inline uint16_t cableId(uint8_t power)
	{
		return CABLE_0 + (power > 15 ? 15 : power);
	}

	inline uint8_t repeaterMode(uint16_t id)
	{
		return isRepeater(id) ? (uint8_t)(id - REPEATER_0) : 0;
	}

	inline uint16_t redirectorId(uint16_t mask)
	{
		return (uint16_t)(REDIRECTOR_0 + (mask & 0x03FFu));
	}

	inline uint16_t redirectorMask(uint16_t id)
	{
		return isRedirector(id) ? (uint16_t)(id - REDIRECTOR_0) : 0;
	}

	inline bool redirectorSideOn(uint16_t id, int side)
	{
		return side >= 0 && side < 10 && (redirectorMask(id) & (1u << side));
	}

	inline uint16_t toggledRedirectorSide(uint16_t id, int side)
	{
		if (side < 0 || side >= 10)
		{
			return redirectorId(redirectorMask(id));
		}
		return redirectorId((uint16_t)(redirectorMask(id) ^ (1u << side)));
	}

	inline bool isSelectable(uint16_t id)
	{
		return id != LEVER_ON && id != CRYSTAL_DOOR_OPEN &&
			(!isCable(id) || id == CABLE_0) &&
			(!isRepeater(id) || id == REPEATER_0) &&
			(!isRedirector(id) || id == REDIRECTOR_0);
	}

	inline uint16_t toggledLever(uint16_t id)
	{
		return id == LEVER_ON ? LEVER_OFF : LEVER_ON;
	}

	inline uint16_t nextRepeaterMode(uint16_t id)
	{
		return isRepeater(id) ? (uint16_t)(REPEATER_0 + ((repeaterMode(id) + 1) & 0x0F)) : REPEATER_0;
	}
}

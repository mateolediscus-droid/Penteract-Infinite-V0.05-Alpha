#pragma once
#include <cstdint>
#include <array>
#include <string_view>

namespace Block
{
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
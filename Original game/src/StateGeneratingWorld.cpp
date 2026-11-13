#include "StateGeneratingWorld.h"
#include "StateGame.h"
#include "Block.h"
#include <random>
#include <boost/lexical_cast.hpp>
#include <fstream>

StateGeneratingWorld StateGeneratingWorld::instanceObj;
StateGeneratingWorld* StateGeneratingWorld::instance()
{
	return &instanceObj;
}

void StateGeneratingWorld::init(StateManager& s)
{
	qr = QuadRenderer{ Shader::get("quad") };
	qr.init();

	generatingText.setText("Generating...");
	generatingText.setAlignX(ui::ALIGN_CENTER_X);
	generatingText.setAlignY(ui::ALIGN_CENTER_Y);
	generatingText.setOffsetY(-50);

	cancelButton.setText("Cancel");
	cancelButton.setAction([this, &s]()
		{
			pool.cancelAllQueuedTasks();
			pool.wait();
			s.popState();
		}
	);
	cancelButton.setSize(110, 50);
	cancelButton.setOffsetX(25);
	cancelButton.setAlignY(ui::ALIGN_BOTTOM);
	cancelButton.setOffsetY(-25);

	page.addElem(&generatingText);
	page.addElem(&cancelButton);
	
	s.setUiPage(&page);

	StateGame* game = StateGame::instance();
	world = game->createWorld(size);

	taskCountTotal = size * size * size * size;
	taskCount = taskCountTotal;

	if (!flat)
	{
		noise = std::make_unique<OpenSimplexNoise>(seed);
	}

	for (int b = 0; b < size; ++b)
	{
		for (int c = 0; c < size; ++c)
		{
			for (int d = 0; d < size; ++d)
			{
				for (int e = 0; e < size; ++e)
				{
					glm::u8vec4 chunkPos{ b, c, d, e };
					Chunk* chunk = world->getChunk(chunkPos);
					if (flat)
					{
						pool.addTask([this, chunk, chunkPos]
							{
								generateChunkFlat(chunk, chunkPos);
								--taskCount;
							}
						);
					}
					else
					{
						pool.addTask([this, chunk, chunkPos]
							{
								generateChunk(chunk, chunkPos);
								--taskCount;
							}
						);
					}
				}
			}
		}
	}
}

void StateGeneratingWorld::update(StateManager& s, double dt)
{
	if (taskCount == 0)
	{
		// only use valid characters for the path name
		std::filesystem::path worldPath = "worlds/";
		if (!std::filesystem::is_directory(worldPath))
		{
			std::filesystem::create_directory(worldPath);
		}

		std::string worldFilename;
		for (int i = 0; i < worldName.length(); ++i)
		{
			char c = tolower(worldName[i]);
			if (isalnum(c) || c == '-' || c == ' ')
			{
				worldFilename.push_back(c);
			}
			else
			{
				worldFilename.push_back('_');
			}
		}

		int counter = 0;
		std::string worldFilenameBase = worldFilename;
		while (std::filesystem::exists(worldPath / worldFilename))
		{
			++counter;
			worldFilename = worldFilenameBase;
			worldFilename.push_back('(');
			worldFilename.append(std::to_string(counter));
			worldFilename.push_back(')');
		}
		worldPath.append(worldFilename);

		// now we have the final directory name
		std::filesystem::create_directory(worldPath);
		StateGame::instance()->setWorldPath(worldPath);

		// laziest file operations
		std::ofstream{ worldPath / "info.json" } << nlohmann::json{
			{ "name" , worldName },
			{ "size" , world->getEdgeLength() },
		};

		std::ofstream{ worldPath / "data.bin", std::ios::binary }.write(
			reinterpret_cast<const char*>(world->getChunks()),
			world->getDataSize()
		);

		// why not
		noise = nullptr;

		// StateGeneratingWorld
		s.popState();

		// StateCreateWorld
		s.popState();

		// StateTitleScreen
		s.popState();

		s.pushState(StateGame::instance());
	}
}

void StateGeneratingWorld::render(StateManager& s)
{
	// render loading bar
	float progress = 1.0f - ((float)taskCount / taskCountTotal);

	int wWidth, wHeight;
	glfwGetFramebufferSize(s.getWindow(), &wWidth, &wHeight);

	int w = glm::max(100, wWidth - 100);
	int h = 20;

	int x = (wWidth - w) / 2;
	int y = (wHeight - h) / 2;

	qr.setMode(QuadRenderer::MODE_FILL);
	
	qr.setPos(x, y, w, h);
	qr.setColor({ 0.0f, 0.0f, 0.0f });
	qr.render();

	qr.setPos(x, y, w * progress, h);
	qr.setColor({ 0.0f, 1.0f, 0.0f });
	qr.render();
}

void StateGeneratingWorld::mouseInput(StateManager& s, double xpos, double ypos)
{
}

void StateGeneratingWorld::mouseButtonInput(StateManager& s, int button, int action, int mods)
{
}

void StateGeneratingWorld::setDetails(std::string_view worldName, std::string_view worldSeed, uint8_t size, bool flat, bool caves)
{
	this->worldName = worldName;

	if (worldSeed.empty())
	{
		// a seed source for the random number engine
		std::random_device rd;
		// mersenne_twister_engine seeded with rd()
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int64_t> distrib{};
		seed = distrib(gen);
	}
	else
	{
		try
		{
			seed = boost::lexical_cast<uint64_t>(worldSeed);
		}
		catch (boost::bad_lexical_cast)
		{
			// hash the seed text
			const char* data = worldSeed.data();
			size_t size = worldSeed.length();

			// FNV-1a hashing algorithm, a short but decent hash function
			uint64_t hash = 0xCBF29CE484222325;
			for (size_t i = 0; i < size; ++i)
			{
				hash ^= data[i];
				hash *= 0x00000100000001B3;
				i++;
			}

			seed = hash;
		}

	}

	this->seed = seed;
	this->size = size;
	this->flat = flat;
	this->caves = caves;
}

void StateGeneratingWorld::generateChunk(Chunk* chunk, glm::u8vec4 chunkPos)
{
	constexpr double NOISE_SCALE = 0.1;

	glm::dvec4 chunkCorner = glm::dvec4{ chunkPos } * (double)Chunk::SIZE;

	int highestRenderer = 0;

	for (int b = 0; b < Chunk::SIZE; ++b)
	{
		for (int c = 0; c < Chunk::SIZE; ++c)
		{
			for (int d = 0; d < Chunk::SIZE; ++d)
			{
				for (int e = 0; e < Chunk::SIZE; ++e)
				{
					Chunk::BlockColumn& column = chunk->blocks[b][c][d][e];

					glm::dvec4 pos = chunkCorner + glm::dvec4{ b, c, d, e };
					pos *= NOISE_SCALE;

					float noiseVal = (noise->Evaluate(pos.x, pos.y, pos.z, pos.w) * 0.5f) + 0.5f;
					int h = glm::round((noiseVal * 20.0f) + 20.0f);

					highestRenderer = glm::max<int>(highestRenderer, h / (Chunk::HEIGHT / 4));

					constexpr auto getBlockType = [](int surfaceDist)
						{
							if (surfaceDist > 0)
							{
								return Block::AIR;
							}
							if (surfaceDist == 0)
							{
								return Block::GRASS;
							}
							if (surfaceDist > -4)
							{
								return Block::DIRT;
							}
							else
							{
								return Block::STONE;
							}
						};

					for (int a = 0; a <= h; ++a)
					{
						int surfaceDist = a - h;

						uint8_t blockType = getBlockType(a - h);

						if (caves)
						{
							float aCave = a * NOISE_SCALE;
							float noiseA = (0.5f * noise->Evaluate(aCave, pos.x, pos.y, pos.z)) + 0.5f;
							float noiseB = (0.5f * noise->Evaluate(aCave, pos.y, pos.z, pos.w)) + 0.5f;

							float caveVal = noiseA * noiseB;
							if (caveVal < 0.2f)
							{
								blockType = Block::AIR;
							}
						}

						int shift = (a & 1) * 4;
						int index = (a >> 1);

						column.blockData[index] |= blockType << shift;
					}
				}
			}
		}
	}

	for (int i = 0; i <= highestRenderer; ++i)
	{
		chunk->shouldUpdateRenderer[i] = true;
	}
}

void StateGeneratingWorld::generateChunkFlat(Chunk* chunk, glm::u8vec4 chunkPos)
{
	for (int b = 0; b < Chunk::SIZE; ++b)
	{
		for (int c = 0; c < Chunk::SIZE; ++c)
		{
			for (int d = 0; d < Chunk::SIZE; ++d)
			{
				for (int e = 0; e < Chunk::SIZE; ++e)
				{
					Chunk::BlockColumn& column = chunk->blocks[b][c][d][e];

					constexpr uint8_t DOUBLE_STONE = Block::STONE | (Block::STONE << 4);
					constexpr uint8_t DOUBLE_DIRT = Block::DIRT | (Block::DIRT << 4);

					std::memset(&column.blockData[0], DOUBLE_STONE, 2);
					std::memset(&column.blockData[2], DOUBLE_DIRT, 2);
					column.blockData[4] = Block::GRASS;
				}
			}
		}
	}

	// all blocks should be in the bottom section
	chunk->shouldUpdateRenderer[0] = true;
}
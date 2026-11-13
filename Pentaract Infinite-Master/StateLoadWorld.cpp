#include "WorldNDRegistry.h"
#include "StateCompileND.h"
#include "StateLoadWorld.h"
#include "StateTitleScreen.h"
#include "StateGame.h"
#include "StateManager.h"
#include "json.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>

StateLoadWorld StateLoadWorld::instanceObj;
StateLoadWorld* StateLoadWorld::instance()
{
	return &instanceObj;
}

void StateLoadWorld::init(StateManager& s)
{
	chooseWorldText.setText("Choose a World...");
	chooseWorldText.setAlignX(ui::ALIGN_CENTER_X);
	chooseWorldText.setOffsetY(50);
	chooseWorldText.setColor(glm::vec4{ 0, 1, 0, 1 });

	cancelButton.setText("Cancel");
	cancelButton.setAction([this, &s]() { s.popState(); });
	cancelButton.setSize(110, 50);
	cancelButton.setOffsetX(25);
	cancelButton.setAlignY(ui::ALIGN_BOTTOM);
	cancelButton.setOffsetY(-25);

	//currentPageText.setText("Page 1/1");
	currentPageText.setAlignX(ui::ALIGN_CENTER_X);
	currentPageText.setAlignY(ui::ALIGN_BOTTOM);
	currentPageText.setOffsetY(-50);
	currentPageText.setColor(glm::vec4{ 0, 1, 0, 1 });

	prevPageButton.setText("<");
	prevPageButton.setAction([this, &s]
		{
			currentPageIndex = glm::max(0, currentPageIndex - 1);
			updatePage(s);
		}
	);
	prevPageButton.setSize(50, 50);
	prevPageButton.setAlignX(ui::ALIGN_CENTER_X);
	prevPageButton.setOffsetX(-100);
	prevPageButton.setAlignY(ui::ALIGN_BOTTOM);
	prevPageButton.setOffsetY(-50);

	nextPageButton.setText(">");
	nextPageButton.setAction([this, &s]
		{
			currentPageIndex = glm::min(pageCount - 1, currentPageIndex + 1);
			updatePage(s);
		}
	);
	nextPageButton.setSize(50, 50);
	nextPageButton.setAlignX(ui::ALIGN_CENTER_X);
	nextPageButton.setOffsetX(100);
	nextPageButton.setAlignY(ui::ALIGN_BOTTOM);
	nextPageButton.setOffsetY(-50);

	// create pages
	for (const auto& entry : std::filesystem::directory_iterator{ "worlds" })
	{
		if (!std::filesystem::is_directory(entry))
		{
			continue;
		}

		// look for info.json
		std::filesystem::path jsonPath = entry.path();
		jsonPath.append("info.json");

		if (std::filesystem::is_regular_file(jsonPath))
		{
			// read the json file
			std::ifstream file{ jsonPath.string() };
			nlohmann::json j;
			file >> j;
			file.close();

			auto worldButton = std::make_unique<ui::button>();

			int index = worldButtons.size();
			int currentPage = index / PAGE_SIZE;

			auto label = j["name"].get<std::string>();
			if (label.size() > 18) label = label.substr(0, 15) + "...";
			worldButton->setText(label + " (" + std::to_string(j.value("dimensions", 5)) + "D)");
			worldButton->setSize(400, 50);
			worldButton->setAlignX(ui::ALIGN_CENTER_X);
			worldButton->setAlignY(ui::ALIGN_TOP);
			worldButton->setOffsetY(100 + ((index % PAGE_SIZE) * 75));

			worldPaths.push_back(entry.path().string());
			worldButtons.emplace_back(std::move(worldButton));
		}
	}

	s.setUiPage(&page);

	currentPageIndex = 0;
	pageCount = glm::max<int>(1, glm::ceil((float)worldPaths.size() / PAGE_SIZE));
	updatePage(s);

	worldLoadFinished = false;
	loading6D = false;
	loading7D = false;
	loading8D = false;
	loading9D = false;
	loadingHigher = nullptr;
}

void StateLoadWorld::close(StateManager& s)
{
	// page must be cleared first
	page.clear();
	s.setUiPage(nullptr);

	worldPaths.clear();
	worldButtons.clear();
}

void StateLoadWorld::pause(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateLoadWorld::resume(StateManager& s)
{
	s.setUiPage(&page);
}

void StateLoadWorld::update(StateManager& s, double dt)
{
	if (worldLoadFinished)
	{
		// StateLoadWorld
		s.popState();

		// StateTitleScreen
		s.popState();

		s.pushState(loadingHigher?loadingHigher:StateGame::instance());
		return;
	}
}

void StateLoadWorld::render(StateManager& s)
{
}

void StateLoadWorld::mouseInput(StateManager& s, double xpos, double ypos)
{
}

void StateLoadWorld::scrollInput(StateManager& s, double xoff, double yoff)
{
}

void StateLoadWorld::mouseButtonInput(StateManager& s, int button, int action, int mods)
{
}

void StateLoadWorld::keyInput(StateManager&, int key, int scancode, int action, int mods)
{
}

void StateLoadWorld::windowResize(StateManager&, int width, int height)
{
}

void StateLoadWorld::updatePage(StateManager& s)
{
	currentPageText.setText("Page " + std::to_string(currentPageIndex + 1) + "/" + std::to_string(pageCount));

	page.clear();
	page.addElem(&cancelButton);
	page.addElem(&chooseWorldText);
	page.addElem(&currentPageText);
	page.addElem(&prevPageButton);
	page.addElem(&nextPageButton);

	int start = currentPageIndex * PAGE_SIZE;
	int end = glm::min<int>(worldPaths.size(), start + PAGE_SIZE);
	for (int i = start; i < end; ++i)
	{
		std::filesystem::path worldPath = worldPaths[i];
		ui::button* worldButton = worldButtons[i].get();
		worldButton->setAction([this, &s, worldPath]
			{
				std::filesystem::path jsonPath = worldPath;
				jsonPath.append("info.json");

				nlohmann::json j;
				std::ifstream{ jsonPath.string() } >> j;

				int edgeLength = j["size"].get<int>();
				uint64_t seed = j.value<uint64_t>("seed", 0);
				bool flat = j.value<bool>("flat", false);
				bool caves = j.value<bool>("caves", false);
				if (j.value("dimensions", 5) >= 6)
				{
					if(!worldNDCompiled(j.value("dimensions",5))){
						StateCompileND::open(s,worldPath,j.value("dimensions",5));return;
					}
					try {
						auto entry=worldNDEntry(j.value("dimensions",5));
						entry.configure(worldPath,seed,flat,caves);loadingHigher=entry.instance();
					}
					catch (const std::exception& error)
					{
						chooseWorldText.setText(std::to_string(j.value("dimensions",5))+"D: " + error.what());
						chooseWorldText.setSize(1);
						chooseWorldText.setWrapWidth(600);
						return;
					}
					loading6D = true;
					loading7D = j.value("dimensions",5)==7;
					loading8D = j.value("dimensions",5)==8;
					loading9D = j.value("dimensions",5)==9;
					worldLoadFinished = true;
					return;
				}
				World* world = StateGame::instance()->createWorld(edgeLength, seed, flat, caves, j.value("dimensions", 5), j.value("renderType", 3));
				StateGame::instance()->setWorldPath(worldPath);

				auto loadChunkFile = [world](const std::filesystem::path& chunksPath)
				{
					std::ifstream chunkFile{ chunksPath, std::ios::binary };
					uint32_t magic = 0;
					uint32_t version = 0;
					uint64_t count = 0;
					chunkFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
					chunkFile.read(reinterpret_cast<char*>(&version), sizeof(version));
					chunkFile.read(reinterpret_cast<char*>(&count), sizeof(count));
					if (magic == 0x50445035 && (version == 1 || version == 2 || version == 3))
					{
						for (uint64_t i = 0; i < count; ++i)
						{
							int32_t coords[5]{};
							uint32_t changeCount = 0;
							if (version == 3)
							{
								chunkFile.read(reinterpret_cast<char*>(coords), sizeof(coords));
							}
							else
							{
								chunkFile.read(reinterpret_cast<char*>(coords), sizeof(int32_t) * 4);
							}
							chunkFile.read(reinterpret_cast<char*>(&changeCount), sizeof(changeCount));
							world->useSavedStructureBaseline({version == 3 ? coords[0] : 0,
								coords[version == 3 ? 1 : 0], coords[version == 3 ? 2 : 1],
								coords[version == 3 ? 3 : 2], coords[version == 3 ? 4 : 3]});

							for (uint32_t j = 0; j < changeCount; ++j)
							{
								World::BlockChange change{};
								if (version == 1)
								{
									struct BlockChangeV1
									{
										uint8_t a;
										uint8_t b;
										uint8_t c;
										uint8_t d;
										uint8_t e;
										uint8_t value;
									};
									BlockChangeV1 oldChange{};
									chunkFile.read(reinterpret_cast<char*>(&oldChange), sizeof(oldChange));
									change = {
										oldChange.a,
										oldChange.b,
										oldChange.c,
										oldChange.d,
										oldChange.e,
										oldChange.value
									};
								}
								else
								{
									chunkFile.read(reinterpret_cast<char*>(&change), sizeof(change));
								}
								world->setBlock(
									m5::ivec5{
										static_cast<int32_t>((version == 3 ? coords[0] : 0) * static_cast<int32_t>(Chunk::HEIGHT) + change.a),
										static_cast<int32_t>(coords[version == 3 ? 1 : 0] * static_cast<int32_t>(Chunk::SIZE) + change.b),
										static_cast<int32_t>(coords[version == 3 ? 2 : 1] * static_cast<int32_t>(Chunk::SIZE) + change.c),
										static_cast<int32_t>(coords[version == 3 ? 3 : 2] * static_cast<int32_t>(Chunk::SIZE) + change.d),
										static_cast<int32_t>(coords[version == 3 ? 4 : 3] * static_cast<int32_t>(Chunk::SIZE) + change.e)
									},
									change.value
								);
							}
						}
					}
					else if (magic == 0x50435035 && version == 1)
					{
						for (uint64_t i = 0; i < count; ++i)
						{
							int32_t coords[4]{};
							Chunk chunk;
							chunkFile.read(reinterpret_cast<char*>(coords), sizeof(coords));
							chunkFile.read(reinterpret_cast<char*>(&chunk), sizeof(Chunk));
							world->setChunk({ 0, coords[0], coords[1], coords[2], coords[3] }, chunk);
						}
					}
				};

				std::filesystem::path chunksDir = worldPath / "chunks";
				if (std::filesystem::is_directory(chunksDir))
				{
					for (const auto& entry : std::filesystem::directory_iterator{ chunksDir })
					{
						if (entry.is_regular_file() && entry.path().extension() == ".bin")
						{
							loadChunkFile(entry.path());
						}
					}
				}
				else if (std::filesystem::is_regular_file(worldPath / "chunks.bin"))
				{
					loadChunkFile(worldPath / "chunks.bin");
				}
				else
				{
					std::filesystem::path dataPath = worldPath / "data.bin";
					std::ifstream dataFile{ dataPath, std::ios::binary };
					if (dataFile.is_open())
					{
						for (int b = 0; b < edgeLength; ++b)
						for (int c = 0; c < edgeLength; ++c)
						for (int d = 0; d < edgeLength; ++d)
						for (int e = 0; e < edgeLength; ++e)
						{
							Chunk chunk;
							dataFile.read(reinterpret_cast<char*>(&chunk), sizeof(Chunk));
							world->setChunk({ 0, b, c, d, e }, chunk);
						}
					}
				}

				// the state change must not be performed here,
				// since closing this state will destroy the lambda functor.
				worldLoadFinished = true;
			}
		);

		page.addElem(worldButton);
	}
}

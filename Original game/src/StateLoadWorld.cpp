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

			worldButton->setText(j["name"].get<std::string>());
			worldButton->setSize(250, 50);
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

		s.pushState(StateGame::instance());
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
				World* world = StateGame::instance()->createWorld(edgeLength);
				StateGame::instance()->setWorldPath(worldPath);

				std::filesystem::path dataPath = worldPath;
				dataPath.append("data.bin");

				std::ifstream{ dataPath, std::ios::binary }.read(
					reinterpret_cast<char*>(world->getChunks()),
					world->getDataSize()
				);

				for (int b = 0; b < edgeLength; ++b)
				{
					for (int c = 0; c < edgeLength; ++c)
					{
						for (int d = 0; d < edgeLength; ++d)
						{
							for (int e = 0; e < edgeLength; ++e)
							{
								glm::u8vec4 chunkPos{ b, c, d, e };
								Chunk* chunk = world->getChunk(chunkPos);

								for (int i = 0; i < 4; ++i)
								{
									chunk->shouldUpdateRenderer[i] = true;
								}
							}
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
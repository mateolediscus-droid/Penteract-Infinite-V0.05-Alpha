#pragma once
#include "State.h"
#include "ui.h"
#include <memory>

class StateLoadWorld : public State
{
public:

	static StateLoadWorld* instance();

	bool shouldRenderPrevState() { return true; }

	void init(StateManager& s) override;
	void close(StateManager& s) override;
	void pause(StateManager& s) override;
	void resume(StateManager& s) override;
	void update(StateManager& s, double dt) override;
	void render(StateManager& s) override;
	void mouseInput(StateManager& s, double xpos, double ypos) override;
	void scrollInput(StateManager& s, double xoff, double yoff) override;
	void mouseButtonInput(StateManager& s, int button, int action, int mods) override;
	void keyInput(StateManager&, int key, int scancode, int action, int mods) override;
	void windowResize(StateManager&, int width, int height) override;

private:

	static StateLoadWorld instanceObj;
	~StateLoadWorld() {}

	ui::page page;

	ui::text chooseWorldText;
	ui::button cancelButton;
	ui::text currentPageText;
	ui::button prevPageButton;
	ui::button nextPageButton;

	std::vector<std::filesystem::path> worldPaths;
	std::vector<std::unique_ptr<ui::button>> worldButtons;

	static constexpr int PAGE_SIZE = 7;
	int currentPageIndex = 0;
	int pageCount = 0;

	bool worldLoadFinished = false;

	void updatePage(StateManager& s);
};
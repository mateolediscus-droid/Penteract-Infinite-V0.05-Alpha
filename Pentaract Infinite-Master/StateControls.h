#pragma once
#include "StateManager.h"
#include "ui.h"

class StateControls : public State
{
public:

	static StateControls* instance();

	bool shouldRenderPrevState() override { return true; }

	void init(StateManager& s) override;
	void close(StateManager& s) override;
	void pause(StateManager& s) override;
	void resume(StateManager& s) override;
	void render(StateManager& s) override;
	void keyInput(StateManager& s, int key, int scancode, int action, int mods) override;
	void windowResize(StateManager& s, int width, int height) override;

private:

	static StateControls instanceObj;
	~StateControls() {}

	ui::page page;

	ui::text titleText;
	ui::text controlsText;
};
#pragma once
#include "StateManager.h"
#include "QuadRenderer.h"
#include "ui.h"

class StateTitleScreen : public State
{
public:

	static StateTitleScreen* instance();

	void init(StateManager& s) override;
	void close(StateManager& s) override;
	void pause(StateManager& s) override;
	void resume(StateManager& s) override;
	void update(StateManager& s, double dt) override;
	void render(StateManager& s) override;
	void mouseInput(StateManager& s, double xpos, double ypos) override;
	void scrollInput(StateManager& s, double xoff, double yoff) override;
	void mouseButtonInput(StateManager& s, int button, int action, int mods) override;
	void keyInput(StateManager& s, int key, int scancode, int action, int mods) override;
	void windowResize(StateManager& s, int width, int height) override;

private:
	
	static StateTitleScreen instanceObj;
	~StateTitleScreen() {}

	ui::page page;

	ui::text text;
	ui::image logo;
	ui::button newWorldButton;
	ui::button loadWorldButton;
	ui::button quitButton;

	QuadRenderer qr;
};
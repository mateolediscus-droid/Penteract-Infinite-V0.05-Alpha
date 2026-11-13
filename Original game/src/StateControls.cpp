#include "StateControls.h"

StateControls StateControls::instanceObj;
StateControls* StateControls::instance()
{
	return &instanceObj;
}

void StateControls::init(StateManager& s)
{
	titleText.setText("Controls:");
	titleText.setSize(3);
	titleText.setAlignX(ui::ALIGN_CENTER_X);
	titleText.setOffsetY(50);

	controlsText.setText(
		"Movement (BC): WASD\n"
		"Strafe Kata (-D): Q\n"
		"Strafe Ana (+D): E\n"
		"Strafe -E: R\n"
		"Strafe +E: F\n"
		"Sprint: CTRL\n"
		"Toggle Collision: C\n"
		"Toggle Flight: V\n"
		"Move Down (Flight) (-A): SHIFT\n"
		"Move Up (Flight) (+A) / Jump: SPACE\n"
		"Select Block: 0-9 or scroll wheel\n"
		"Break Block: left-click\n"
		"Place Block: right-click\n"
		"Look BC/AB: mouse\n"
		"Look CD/BD: middle-click + mouse\n"
		"Look CE/BE: ALT + middle-click + mouse\n"
		"Open Console: \"`\" key (right next to the \"1\" key)\n"
		"Save: CTRL + SHIFT + S\n"
	);
	controlsText.setAlignX(ui::ALIGN_CENTER_X);
	controlsText.setOffsetY(100);

	page.clear();
	page.addElem(&titleText);
	page.addElem(&controlsText);

	s.setUiPage(&page);

	int wWidth, wHeight;
	glfwGetFramebufferSize(s.getWindow(), &wWidth, &wHeight);
	windowResize(s, wWidth, wHeight);
}

void StateControls::close(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateControls::pause(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateControls::resume(StateManager& s)
{
	s.setUiPage(&page);
}

void StateControls::render(StateManager& s)
{
}

void StateControls::keyInput(StateManager& s, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		s.popState();
	}
}

void StateControls::windowResize(StateManager& s, int width, int height)
{
	controlsText.setWrapWidth(width - 100);
}
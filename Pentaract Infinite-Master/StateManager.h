#pragma once
#include "State.h"
#include "Math5D.h"
#include "ui.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

class StateManager
{
public:

	StateManager(GLFWwindow* w) : window{ w } {}

	bool isRunning();
	State* getCurrentState();
	State* getPrevState();

	GLFWwindow* getWindow();

	void update(double dt);
	void render();

	void quit();

	void mouseInput(double xpos, double ypos);
	void scrollInput(double xoffset, double yoffset);
	void mouseButtonInput(int button, int action, int mods);
	void keyInput(int key, int scancode, int action, int mods);
	void charInput(unsigned int codepoint);
	void fileDrop(int path_count, const char* paths[]);

	void windowResize(int width, int height);

	ui::page* getUiPage();
	void setUiPage(ui::page* page);

	void pushState(State* state);
	void popState();
	void changeState(State* newState);

	bool shouldClose() const;
	void getSize(int* width, int* height) const;

private:

	std::vector<State*> states{};
	GLFWwindow* window = nullptr;
	ui::page* page = nullptr;
};
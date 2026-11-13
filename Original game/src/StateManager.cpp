#include "StateManager.h"

bool StateManager::isRunning()
{
	return !states.empty();
}
State* StateManager::getCurrentState()
{
	if (states.empty()) return nullptr;
	return states.back();
}
State* StateManager::getPrevState()
{
	if (states.size() < 2) return nullptr;
	return states[states.size() - 2];
}

GLFWwindow* StateManager::getWindow()
{
	return window;
}

void StateManager::update(double dt)
{
	if (isRunning() && !states.empty())
	{
		// an index must be used here because updates can add or remove states,
		// invalidating any iterators.
		// this is technically possible in render(),
		// but who would modify the StateManager from a render method?
		int index = states.size();
		do
		{
			if (index == 0)
			{
				break;
			}
			--index;
		} while (states[index]->shouldUpdatePrevState());

		for (; index < states.size(); ++index)
		{
			states[index]->update(*this, dt);
		}
	}
}

void StateManager::render()
{
	if (isRunning() && !states.empty())
	{
		auto it = states.end();
		do
		{
			if (it == states.begin())
			{
				break;
			}
			--it;
		} while ((*it)->shouldRenderPrevState());

		for (; it != states.end(); ++it)
		{
			State* state = *it;
			state->render(*this);
		}
	}

	if (page != nullptr)
	{
		page->render();
	}

	glfwSwapBuffers(window);
}

void StateManager::quit()
{
	glfwSetWindowShouldClose(window, true);
}

bool StateManager::shouldClose() const
{
	return glfwWindowShouldClose(window);
}

void StateManager::mouseInput(double xpos, double ypos)
{
	if (page != nullptr)
	{
		page->mouseInput(xpos, ypos);
		return;
	}

	if (isRunning())
	{
		getCurrentState()->mouseInput(*this, xpos, ypos);
	}
}

void StateManager::scrollInput(double xoffset, double yoffset)
{
	if (page != nullptr && page->scrollInput(xoffset, yoffset))
	{
		return;
	}

	if (isRunning())
	{
		getCurrentState()->scrollInput(*this, xoffset, yoffset);
	}
}

void StateManager::mouseButtonInput(int button, int action, int mods)
{
	if (page != nullptr && page->mouseButtonInput(button, action, mods))
	{
		return;
	}

	if (isRunning())
	{
		getCurrentState()->mouseButtonInput(*this, button, action, mods);
	}
}

void StateManager::keyInput(int key, int scancode, int action, int mods)
{
	if (page != nullptr && page->keyInput(key, scancode, action, mods))
	{
		return;
	}

	if (isRunning())
	{
		getCurrentState()->keyInput(*this, key, scancode, action, mods);
	}
}

void StateManager::windowResize(int width, int height)
{
	width = glm::max(width, 1);
	height = glm::max(height, 1);
	
	if (page != nullptr)
	{
		page->windowResize(width, height);
	}

	for (auto& state : states)
	{
		state->windowResize(*this, width, height);
	}

	render();
}

void StateManager::charInput(unsigned int codepoint)
{
	if (page != nullptr && page->charInput(codepoint))
	{
		return;
	}

	if (isRunning())
	{
		getCurrentState()->charInput(*this, codepoint);
	}
}

void StateManager::fileDrop(int path_count, const char* paths[])
{
	if (isRunning())
	{
		getCurrentState()->fileDrop(*this, path_count, paths);
	}
}

ui::page* StateManager::getUiPage()
{
	return page;
}

void StateManager::setUiPage(ui::page* page)
{
	this->page = page;
	if (page != nullptr)
	{
		page->init(window);
	}
}

void StateManager::pushState(State* state)
{
	if (!states.empty())
	{
		states.back()->pause(*this);
	}

	states.emplace_back(state);

	state->init(*this);
}

void StateManager::popState()
{
	if (!states.empty())
	{
		states.back()->close(*this);

		states.pop_back();

		if (!states.empty())
		{
			states.back()->resume(*this);
		}
	}
}

void StateManager::changeState(State* newState)
{
	popState();
	pushState(newState);
}

void StateManager::getSize(int* width, int* height) const
{
	glfwGetFramebufferSize(window, width, height);
}
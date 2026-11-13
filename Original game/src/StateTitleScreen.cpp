#include "StateTitleScreen.h"
#include "StateCreateWorld.h"
#include "StateLoadWorld.h"
#include "audio.h"
#include <iostream>

StateTitleScreen StateTitleScreen::instanceObj;
StateTitleScreen* StateTitleScreen::instance()
{
	return &instanceObj;
}

void StateTitleScreen::init(StateManager& s)
{
	qr = QuadRenderer{ Shader::get("quad") };
	qr.init();

	//text.setValue("hello world");
	//text.setAlignX(ui::ALIGN_CENTER_X);
	//text.setOffsetY(300);
	//text.setColor(glm::vec4{ 0, 1, 0, 1 });

	Texture::load("assets/textures/logo.png", 0, false, "logo.png");
	logo.setTexture(Texture::get("logo.png"));
	logo.setAlignX(ui::ALIGN_CENTER_X);
	logo.setScale(2.0f, 2.0f);

	newWorldButton.setText("New World");
	newWorldButton.setAction([this, &s]() { s.pushState(StateCreateWorld::instance()); });
	newWorldButton.setSize(160, 50);
	newWorldButton.setAlignX(ui::ALIGN_CENTER_X);
	newWorldButton.setOffsetY(350);

	loadWorldButton.setText("Load World");
	loadWorldButton.setAction([this, &s]() { s.pushState(StateLoadWorld::instance()); });
	loadWorldButton.setSize(180, 50);
	loadWorldButton.setAlignX(ui::ALIGN_CENTER_X);
	loadWorldButton.setOffsetY(450);

	quitButton.setText("Quit Game");
	quitButton.setAction([this, &s]() { s.popState(); });
	quitButton.setSize(170, 50);
	quitButton.setAlignX(ui::ALIGN_CENTER_X);
	quitButton.setOffsetY(550);

	page.clear();
	page.addElem(&text);
	page.addElem(&logo);
	page.addElem(&newWorldButton);
	page.addElem(&loadWorldButton);
	page.addElem(&quitButton);

	s.setUiPage(&page);

	audio::clearBgm();
	audio::loadSound("music/Extended Intro.mp3");
	audio::addToBgmList("music/Extended Intro.mp3");
}

void StateTitleScreen::close(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateTitleScreen::pause(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateTitleScreen::resume(StateManager& s)
{
	s.setUiPage(&page);
}

void StateTitleScreen::update(StateManager& s, double dt)
{
	audio::updateBgm();
}

void StateTitleScreen::render(StateManager& s)
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	int wW, wH;
	s.getSize(&wW, &wH);

	qr.setMode(QuadRenderer::MODE_FILL);
	qr.setColors({
		glm::vec4{ 0.3f, 0.14f, 0.34f, 1.0f },
		glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f },
		glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f },
		glm::vec4{ 0.3f, 0.14f, 0.34f, 1.0f }
	});
	qr.setPos(0, 0, wW, wH);
	qr.render();
}

void StateTitleScreen::mouseInput(StateManager& s, double xpos, double ypos)
{
}

void StateTitleScreen::scrollInput(StateManager& s, double xoff, double yoff)
{
}

void StateTitleScreen::mouseButtonInput(StateManager& s, int button, int action, int mods)
{
}

void StateTitleScreen::keyInput(StateManager& s, int key, int scancode, int action, int mods)
{
}

void StateTitleScreen::windowResize(StateManager& s, int width, int height)
{
}
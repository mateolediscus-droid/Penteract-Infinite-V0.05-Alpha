#include "StateCreateWorld.h"
#include "StateTitleScreen.h"
#include "StateGeneratingWorld.h"
#include "StateManager.h"
#include <iostream>
#include <filesystem>

StateCreateWorld StateCreateWorld::instanceObj;
StateCreateWorld* StateCreateWorld::instance()
{
	return &instanceObj;
}

void StateCreateWorld::init(StateManager& s)
{
	worldNameText.setText("World Name:");
	worldNameText.setAlignX(ui::ALIGN_CENTER_X);
	worldNameText.setOffsetY(50);
	worldNameText.setColor(glm::vec4{ 0, 1, 0, 1 });

	worldNameInput.setText("My World");
	worldNameInput.setSize(250, 50);
	worldNameInput.setAlignX(ui::ALIGN_CENTER_X);
	worldNameInput.setOffsetY(75);

	worldSeedText.setText("World Seed:");
	worldSeedText.setAlignX(ui::ALIGN_CENTER_X);
	worldSeedText.setOffsetY(150);
	worldSeedText.setColor(glm::vec4{ 0, 1, 0, 1 });

	worldSeedInput.setText("");
	worldSeedInput.setSize(250, 50);
	worldSeedInput.setAlignX(ui::ALIGN_CENTER_X);
	worldSeedInput.setOffsetY(175);

	flatCheckbox.setText("Flat World");
	flatCheckbox.setChecked(false);
	flatCheckbox.setAction([this, &s](bool checked)
		{
			if (checked)
			{
				page.removeElem(&cavesCheckbox);
			}
			else
			{
				page.addElem(&cavesCheckbox);
			}
		}
	);
	flatCheckbox.setAlignX(ui::ALIGN_CENTER_X);
	flatCheckbox.setOffsetY(275);

	cavesCheckbox.setText("Caves");
	cavesCheckbox.setChecked(false);
	cavesCheckbox.setAction([this, &s](bool checked) { std::cout << (checked ? "checked" : "unchecked") << std::endl; });
	cavesCheckbox.setAlignX(ui::ALIGN_CENTER_X);
	cavesCheckbox.setOffsetY(325);

	worldSizeLabelText.setText("World Size:");
	worldSizeLabelText.setAlignX(ui::ALIGN_CENTER_X);
	worldSizeLabelText.setOffsetY(400);
	worldSizeLabelText.setColor(glm::vec4{ 0, 1, 0, 1 });

	worldSize = 3;
	worldSizeValueText.setText(std::to_string(worldSize));
	worldSizeValueText.setAlignX(ui::ALIGN_CENTER_X);
	worldSizeValueText.setOffsetY(440);

	worldSizeDecButton.setText("<");
	worldSizeDecButton.setAction([this, &s]()
		{
			worldSize = glm::max(1, worldSize - 1);
			worldSizeValueText.setText(std::to_string(worldSize));
		}
	);
	worldSizeDecButton.setSize(50, 50);
	worldSizeDecButton.setAlignX(ui::ALIGN_CENTER_X);
	worldSizeDecButton.setOffsetX(-50);
	worldSizeDecButton.setOffsetY(425);

	worldSizeIncButton.setText(">");
	worldSizeIncButton.setAction([this, &s]()
		{
			worldSize = glm::min(8, worldSize + 1);
			worldSizeValueText.setText(std::to_string(worldSize));
		}
	);
	worldSizeIncButton.setSize(50, 50);
	worldSizeIncButton.setAlignX(ui::ALIGN_CENTER_X);
	worldSizeIncButton.setOffsetX(50);
	worldSizeIncButton.setOffsetY(425);

	cancelButton.setText("Cancel");
	cancelButton.setAction([this, &s]() { s.popState(); });
	cancelButton.setSize(110, 50);
	cancelButton.setOffsetX(25);
	cancelButton.setAlignY(ui::ALIGN_BOTTOM);
	cancelButton.setOffsetY(-25);

	createButton.setText("Create New World!");
	createButton.setAction([this, &s]()
		{
			std::string worldName = worldNameInput.getText();

			if (worldName.empty())
			{
				return;
			}

			StateGeneratingWorld::instance()->setDetails(worldName, worldSeedInput.getText(), worldSize, flatCheckbox.getChecked(), cavesCheckbox.getChecked());
			s.pushState(StateGeneratingWorld::instance());
		}
	);
	createButton.setSize(290, 50);
	createButton.setAlignX(ui::ALIGN_RIGHT);
	createButton.setOffsetX(-25);
	createButton.setAlignY(ui::ALIGN_BOTTOM);
	createButton.setOffsetY(-25);

	page.clear();
	page.addElem(&worldNameText);
	page.addElem(&worldNameInput);
	page.addElem(&worldSeedText);
	page.addElem(&worldSeedInput);
	page.addElem(&flatCheckbox);
	page.addElem(&cavesCheckbox);
	page.addElem(&worldSizeLabelText);
	page.addElem(&worldSizeValueText);
	page.addElem(&worldSizeDecButton);
	page.addElem(&worldSizeIncButton);
	page.addElem(&cancelButton);
	page.addElem(&createButton);

	s.setUiPage(&page);
}

void StateCreateWorld::close(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateCreateWorld::pause(StateManager& s)
{
	s.setUiPage(nullptr);
}

void StateCreateWorld::resume(StateManager& s)
{
	s.setUiPage(&page);
}

void StateCreateWorld::update(StateManager& s, double dt)
{
}

void StateCreateWorld::render(StateManager& s)
{
	//glClearColor(0, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glDisable(GL_DEPTH_TEST);

	//int wW, wH;
	//s.getSize(&wW, &wH);

	//qr.setMode(QuadRenderer::MODE_FILL);
	//qr.setColors({
	//	glm::vec4{ 0.3f, 0.14f, 0.34f, 1.0f },
	//	glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f },
	//	glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f },
	//	glm::vec4{ 0.3f, 0.14f, 0.34f, 1.0f }
	//});
	//qr.setPos(0, 0, wW, wH);
	//qr.render();
}

void StateCreateWorld::mouseInput(StateManager& s, double xpos, double ypos)
{
}

void StateCreateWorld::scrollInput(StateManager& s, double xoff, double yoff)
{
}

void StateCreateWorld::mouseButtonInput(StateManager& s, int button, int action, int mods)
{
}

void StateCreateWorld::keyInput(StateManager&, int key, int scancode, int action, int mods)
{
}

void StateCreateWorld::windowResize(StateManager&, int width, int height)
{
}
#include "StateCreateWorld.h"
#include "StateTitleScreen.h"
#include "StateGeneratingWorld.h"
#include "StateManager.h"
#include "WorldRenderType.h"
#include "TemporalDimensions.h"
#include "WorldNDRegistry.h"
#include <iostream>
#include <filesystem>
// sorry
#include <Windows.h>
#undef min;
#undef max;

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

	dimensions = 5;
	dimensionsLabel.setText("Dimensions:");
	dimensionsLabel.setAlignX(ui::ALIGN_CENTER_X);
	dimensionsLabel.setOffsetY(400);
	dimensionsLabel.setOffsetX(0);
	dimensionsLabel.setColor({0, 1, 0, 1});
	dimensionsValue.setText("5");
	dimensionsValue.setSize(150,50);
	dimensionsValue.setAlignX(ui::ALIGN_CENTER_X);
	dimensionsValue.setOffsetY(425);
	dimensionsValue.setOffsetX(0);
	for (auto button : {&dimensionsDec, &dimensionsInc})
	{
		button->setSize(50, 50);
		button->setAlignX(ui::ALIGN_CENTER_X);
		button->setOffsetY(425);
	}
	dimensionsDec.setText("<");
	dimensionsDec.setOffsetX(-125);
	dimensionsDec.setAction([this] {
		dimensions = glm::max(2, dimensions - 1);
		dimensionsValue.setText(std::to_string(dimensions));
		refreshRenderType();
		refreshTemporalDimensions();
	});
	dimensionsInc.setText(">");
	dimensionsInc.setOffsetX(125);
	dimensionsInc.setAction([this] {
		dimensions = glm::min(maxSpatialDimensions, dimensions + 1);
		dimensionsValue.setText(std::to_string(dimensions));
		refreshRenderType();
		refreshTemporalDimensions();
	});

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
			if(!readDimensions()) {
				MessageBoxA(nullptr,"Enter an integer >= 2 within the engine's integer range.","Invalid dimension",MB_OK|MB_ICONWARNING);
				return;
			}

			if (worldName.empty())
			{
				MessageBox(
					NULL,
					(LPCWSTR)L"Error: you must enter a world name!!!!!",
					(LPCWSTR)L"Error",
					MB_ICONWARNING | MB_OK
				);
				return;
			}

			StateGeneratingWorld::instance()->setDetails(worldName, worldSeedInput.getText(), 3, flatCheckbox.getChecked(), cavesCheckbox.getChecked(), dimensions, renderType, temporalDimensions);
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
	page.addElem(&dimensionsLabel);
	page.addElem(&dimensionsValue);
	page.addElem(&dimensionsDec);
	page.addElem(&dimensionsInc);
	page.addElem(&cancelButton);
	page.addElem(&createButton);
	renderType = 3;
	renderTypeLabel.setText("Render Type:");
	renderTypeLabel.setAlignX(ui::ALIGN_CENTER_X);
	renderTypeLabel.setOffsetX(-140);
	renderTypeLabel.setOffsetY(365);
	renderTypeLabel.setColor({0,1,0,1});
	page.addElem(&renderTypeLabel);
	render2D.setText("2D");
	render3D.setText("3D");
	for (auto box : {&render2D, &render3D})
	{
		box->setAlignX(ui::ALIGN_CENTER_X);
		box->setOffsetY(360);
	}
	render2D.setOffsetX(40);
	render3D.setOffsetX(150);
	render2D.setAction([this](bool checked) { if (checked) renderType = 2; refreshRenderType(); });
	render3D.setAction([this](bool checked) { if (checked) renderType = 3; refreshRenderType(); });
	refreshRenderType();
	temporalDimensions = 1;
	temporalLabel.setText("Temporal ND:");
	temporalLabel.setAlignX(ui::ALIGN_CENTER_X);
	temporalLabel.setOffsetX(-140);
	temporalLabel.setOffsetY(499);
	temporalLabel.setColor({0,1,0,1});
	temporalValue.setAlignX(ui::ALIGN_CENTER_X);
	temporalValue.setOffsetX(80);
	temporalValue.setOffsetY(499);
	for (auto button : {&temporalDec, &temporalInc})
	{
		button->setSize(36,36);
		button->setAlignX(ui::ALIGN_CENTER_X);
		button->setOffsetY(490);
	}
	temporalDec.setText("<"); temporalDec.setOffsetX(20);
	temporalInc.setText(">"); temporalInc.setOffsetX(140);
	temporalDec.setAction([this] { --temporalDimensions; refreshTemporalDimensions(); });
	temporalInc.setAction([this] { ++temporalDimensions; refreshTemporalDimensions(); });
	page.addElem(&temporalLabel);
	page.addElem(&temporalValue);
	refreshTemporalDimensions();

	s.setUiPage(&page);
}

void StateCreateWorld::refreshRenderType()
{
	renderType = worldRenderType(dimensions, renderType);
	page.removeElem(&render2D);
	page.removeElem(&render3D);
	page.addElem(&render2D);
	if (dimensions >= 3) page.addElem(&render3D);
	render2D.setChecked(renderType == 2);
	render3D.setChecked(renderType == 3);
}

void StateCreateWorld::refreshTemporalDimensions()
{
	temporalDimensions = temporalDimensionsFor(dimensions, temporalDimensions);
	temporalValue.setText(std::to_string(temporalDimensions) + "D");
	page.removeElem(&temporalDec);
	page.removeElem(&temporalInc);
	if (maxTemporalDimensions(dimensions) > 1)
	{
		page.addElem(&temporalDec);
		page.addElem(&temporalInc);
	}
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

bool StateCreateWorld::readDimensions()
{
	try {
		auto text=dimensionsValue.getText();
		if(text.empty() || text.find_first_not_of("0123456789")!=std::string::npos)return false;
		int value=std::stoi(text);
		if(value<2 || value>maxSpatialDimensions)return false;
		if(dimensions!=value){dimensions=value;refreshRenderType();refreshTemporalDimensions();}
		return true;
	}catch(const std::exception&){return false;}
}

void StateCreateWorld::update(StateManager& s, double dt)
{
	readDimensions();
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

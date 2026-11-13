#include "StateGame.h"
#include "StateTitleScreen.h"
#include "StateControls.h"
#include "Shader.h"
#include "Texture.h"
#include "TextureBuffer.h"
#include "QuadRendererBasic.h"
#include "Directory.h"
#include "utils.h"
#include "audio.h"
#include "File.h"
#include <chrono>

StateGame StateGame::instanceObj;
StateGame* StateGame::instance()
{
	return &instanceObj;
}

void StateGame::saveSettings() const
{
	std::ofstream file{ "settings.json" };

	if (!file.is_open())
	{
		return;
	}

	nlohmann::json j
	{
		{ "water", waterCheckbox.getChecked() },
		{ "ao", ambientOcclusionCheckbox.getChecked() },
		{ "shadows", shadowsCheckbox.getChecked() },
		{ "scaleUIx2", scaleUIx2Checkbox.getChecked() },
	};

	file << j.dump(1, '\t');

	file.close();
}

void StateGame::loadSettings()
{
	if (!std::filesystem::exists("settings.json"))
	{
		saveSettings();
	}

	std::ifstream file{ "settings.json" };
	if (!file.is_open())
	{
		return;
	}

	nlohmann::json j = nlohmann::json::parse(file);
	file.close();

	try
	{
		if (j.contains("water") && j["water"].is_boolean())
		{
			waterCheckbox.setChecked(j["water"]);
		}
		else
		{
			waterCheckbox.setChecked(true);
		}

		if (j.contains("ao") && j["ao"].is_boolean())
		{
			ambientOcclusionCheckbox.setChecked(j["ao"]);
		}
		else
		{
			ambientOcclusionCheckbox.setChecked(true);
		}

		if (j.contains("shadows") && j["shadows"].is_boolean())
		{
			shadowsCheckbox.setChecked(j["shadows"]);
		}
		else
		{
			shadowsCheckbox.setChecked(true);
		}

		if (j.contains("scaleUIx2") && j["scaleUIx2"].is_boolean())
		{
			scaleUIx2Checkbox.setChecked(j["scaleUIx2"]);
		}
		else
		{
			scaleUIx2Checkbox.setChecked(false);
		}
	}
	catch (const nlohmann::json::exception&)
	{

	}
}

void StateGame::init(StateManager& s)
{
	qr = QuadRenderer{ Shader::get("quad") };
	qr.init();

	rendererShader = Shader::get("renderer");

	cameraBuf = GPUBuffer{ sizeof(Camera) };
	cameraBuf.use(0);

	cam.pos = { 18.0f, 3.5f, 3.5f, 3.5f, 3.5f };
	cam.up = m5::vec5::up();
	cam.forward = m5::vec5::forward();
	cam.left = m5::vec5::left();
	cam.over = m5::vec5::over();
	cam.yonder = m5::vec5::yonder();
	cam.vFov = glm::radians(90.0f);
	orientation = {};

	disableCursor(s.getWindow());

	int wWidth, wHeight;
	s.getSize(&wWidth, &wHeight);

	{
		pausedText.setText("Paused...");
		pausedText.setSize(3);
		pausedText.setShadow(true);
		pausedText.setAlignX(ui::ALIGN_CENTER_X);
		pausedText.setOffsetY(200);

		backToGameButton.setText("Back To Game");
		backToGameButton.setAction([this, &s]()
			{
				disableCursor(s.getWindow());
				s.setUiPage(nullptr);
				paused = false;
			}
		);
		backToGameButton.setSize(210, 50);
		backToGameButton.setAlignX(ui::ALIGN_CENTER_X);
		backToGameButton.setOffsetY(350);

		controlsButton.setText("Controls");
		controlsButton.setAction([this, &s]()
			{
				s.pushState(StateControls::instance());
			}
		);
		controlsButton.setSize(230, 50);
		controlsButton.setAlignX(ui::ALIGN_CENTER_X);
		controlsButton.setOffsetY(450);

		quitToTitleButton.setText("Quit To Title");
		quitToTitleButton.setAction([this, &s]()
			{
				s.changeState(StateTitleScreen::instance());
			}
		);
		quitToTitleButton.setSize(230, 50);
		quitToTitleButton.setAlignX(ui::ALIGN_CENTER_X);
		quitToTitleButton.setOffsetY(550);

		shadowsCheckbox.setText("Shadows");
		shadowsCheckbox.setChecked(true);
		shadowsCheckbox.setAlignX(ui::ALIGN_LEFT);
		shadowsCheckbox.setAlignY(ui::ALIGN_BOTTOM);
		shadowsCheckbox.setOffsetX(10);
		shadowsCheckbox.setOffsetY(-100);

		ambientOcclusionCheckbox.setText("Ambient Occlusion");
		ambientOcclusionCheckbox.setChecked(true);
		ambientOcclusionCheckbox.setAlignX(ui::ALIGN_LEFT);
		ambientOcclusionCheckbox.setAlignY(ui::ALIGN_BOTTOM);
		ambientOcclusionCheckbox.setOffsetX(10);
		ambientOcclusionCheckbox.setOffsetY(-75);

		waterCheckbox.setText("Water");
		waterCheckbox.setChecked(true);
		waterCheckbox.setAlignX(ui::ALIGN_LEFT);
		waterCheckbox.setAlignY(ui::ALIGN_BOTTOM);
		waterCheckbox.setOffsetX(10);
		waterCheckbox.setOffsetY(-50);

		scaleUIx2Checkbox.setText("Scale UI x2");
		scaleUIx2Checkbox.setChecked(false);
		scaleUIx2Checkbox.setAlignX(ui::ALIGN_LEFT);
		scaleUIx2Checkbox.setAlignY(ui::ALIGN_BOTTOM);
		scaleUIx2Checkbox.setOffsetX(10);
		scaleUIx2Checkbox.setOffsetY(-25);

		loadSettings();

		pauseMenu.clear();
		pauseMenu.addElem(&pausedText);
		pauseMenu.addElem(&backToGameButton);
		pauseMenu.addElem(&controlsButton);
		pauseMenu.addElem(&quitToTitleButton);
		pauseMenu.addElem(&shadowsCheckbox);
		pauseMenu.addElem(&ambientOcclusionCheckbox);
		pauseMenu.addElem(&waterCheckbox);
		pauseMenu.addElem(&scaleUIx2Checkbox);

		paused = false;
	}

	{
		fpsText.setText("");
		fpsText.setSize(2);
		fpsText.setShadow(true);
		fpsText.setAlignX(ui::ALIGN_LEFT);
		fpsText.setAlignY(ui::ALIGN_TOP);
		fpsText.setOffsetX(5);
		fpsText.setOffsetY(5);

		coordsText.setText("");
		coordsText.setSize(1);
		coordsText.setShadow(true);
		coordsText.setAlignX(ui::ALIGN_LEFT);
		coordsText.setAlignY(ui::ALIGN_TOP);
		coordsText.setOffsetX(5);
		coordsText.setOffsetY(25);

		selectedBlockText.setAlignX(ui::ALIGN_RIGHT);
		selectedBlockText.setOffsetX(-100);
		selectedBlockText.setAlignY(ui::ALIGN_BOTTOM);
		selectedBlockText.setOffsetY(-45);

		Texture::load("assets/textures/blockIcons.png", 0, false, "blockIcons.png");
		selectedBlockImage.setScale(2, 2);
		selectedBlockImage.setTexture(Texture::get("blockIcons.png"));
		selectedBlockImage.setAlignX(ui::ALIGN_RIGHT);
		selectedBlockImage.setOffsetX(-50);
		selectedBlockImage.setAlignY(ui::ALIGN_BOTTOM);
		selectedBlockImage.setOffsetY(-50);

		selectBlock(Block::GRASS);

		ui.clear();
		ui.addElem(&fpsText);
		ui.addElem(&coordsText);
		ui.addElem(&selectedBlockText);
		ui.addElem(&selectedBlockImage);
		ui.init(s.getWindow());
	}

	{
		console.qr = QuadRenderer{ Shader::get("quad") };
		console.qr.init();

		console.inputText.setShadow(false);
		console.inputText.setAlignX(ui::ALIGN_LEFT);
		console.inputText.setAlignY(ui::ALIGN_TOP);
		console.inputText.setOffsetX(5);
		console.inputText.setOffsetY(console.height - 8 - 5);
		console.inputText.setSize(1);

		console.logText.setShadow(false);
		console.logText.setAlignX(ui::ALIGN_LEFT);
		console.logText.setAlignY(ui::ALIGN_TOP);
		console.logText.setOffsetX(5);
		console.logText.setOffsetY(console.height - 8 * 2 - 5);
		console.logText.setSize(1);
		console.logText.setWrapWidth(wWidth);

		updateConsoleInput();

		console.ui.clear();
		console.ui.addElem(&console.inputText);
		console.ui.addElem(&console.logText);
	}

	//createWorld(3);

	// load player save data if it exists
	{
		std::filesystem::path playerPath = worldPath / "player.json";
		if (std::filesystem::is_regular_file(playerPath))
		{
			nlohmann::json j;
			std::ifstream{ playerPath } >> j;

			cam.pos = j["pos"];
			orientation = j["orientation"];
		}
		else
		{
			cam.pos = {
				32.0f,
				world.getEdgeLength() * Chunk::SIZE / 2.0f,
				world.getEdgeLength() * Chunk::SIZE / 2.0f,
				world.getEdgeLength() * Chunk::SIZE / 2.0f,
				world.getEdgeLength() * Chunk::SIZE / 2.0f
			};
			orientation = {};
		}
		updateCamDirs();
	}

	//for (int e = 0; e < world.getEdgeLength() * Chunk::SIZE; ++e)
	//{
	//	for (int d = 0; d < world.getEdgeLength() * Chunk::SIZE; ++d)
	//	{
	//		for (int c = 0; c < world.getEdgeLength() * Chunk::SIZE; ++c)
	//		{
	//			for (int b = 0; b < world.getEdgeLength() * Chunk::SIZE; ++b)
	//			{
	//				for (int a = 0; a < 16; ++a)
	//				{
	//					if (a >= 15)
	//					{
	//						world.setBlock({ a,b,c,d,e }, Block::GRASS);
	//					}
	//					else if (a >= 12)
	//					{
	//						world.setBlock({ a,b,c,d,e }, Block::DIRT);
	//					}
	//					else if (a >= 0)
	//					{
	//						world.setBlock({ a,b,c,d,e }, Block::STONE);
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	// init renderer buffers
	chunksBuffer = TextureBuffer{ 1, 1, 1, TextureFormat::RGBA32u };
	blockDataHandlesBuffer = GPUBuffer{ sizeof(uint64_t) };
	std::array<int, 10 * Block::COUNT> allTextures{ 0 };
	for (int i = 0; i < Block::COUNT; ++i)
	{
		std::memcpy(allTextures.data() + i * 10, Block::blocks[i].textures, sizeof(Block::blocks[i].textures));
	}
	blockTexturesBuffer = GPUBuffer{ sizeof(allTextures), allTextures.data() };

	// TODO: maybe handle this with a json file or something
	std::vector<uint64_t> textures{};
	Texture::load("assets/textures/blocks/dirt.ndtf", 4, false, "blocks.dirt", true); // 0
	Texture::load("assets/textures/blocks/grassSide.ndtf", 4, false, "blocks.grassSide", true); // 1
	Texture::load("assets/textures/blocks/grassTop.ndtf", 4, false, "blocks.grassTop", true); // 2
	Texture::load("assets/textures/blocks/stone.ndtf", 4, false, "blocks.stone", true); // 3
	Texture::load("assets/textures/blocks/woodSide.ndtf", 4, false, "blocks.woodSide", true); // 4
	Texture::load("assets/textures/blocks/woodTop.ndtf", 4, false, "blocks.woodTop", true); // 5
	Texture::load("assets/textures/blocks/glass.ndtf", 4, false, "blocks.glass", true); // 6
	Texture::load("assets/textures/blocks/bricks.ndtf", 4, false, "blocks.bricks", true); // 7
	Texture::load("assets/textures/blocks/water.ndtf", 4, false, "blocks.water", true); // 8
	textures.emplace_back(Texture::get("blocks.dirt")->getHandle());
	textures.emplace_back(Texture::get("blocks.grassSide")->getHandle());
	textures.emplace_back(Texture::get("blocks.grassTop")->getHandle());
	textures.emplace_back(Texture::get("blocks.stone")->getHandle());
	textures.emplace_back(Texture::get("blocks.woodSide")->getHandle());
	textures.emplace_back(Texture::get("blocks.woodTop")->getHandle());
	textures.emplace_back(Texture::get("blocks.glass")->getHandle());
	textures.emplace_back(Texture::get("blocks.bricks")->getHandle());
	textures.emplace_back(Texture::get("blocks.water")->getHandle());
	tileTextureHandles = GPUBuffer{ sizeof(uint64_t) * textures.size(), textures.data() };

	updateRendererData();
	
	if (console.open)
	{
		if (glfwGetInputMode(s.getWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		{
			enableCursor(s.getWindow());
		}
		if (s.getUiPage() != &console.ui)
		{
			s.setUiPage(&console.ui);
		}
	}

	audio::clearBgm();
	audio::loadSound("music/Not spook.mp3");
	audio::addToBgmList("music/Not spook.mp3");
	audio::loadSound("music/Spook ambient.mp3");
	audio::addToBgmList("music/Spook ambient.mp3");
	audio::loadSound("music/kind of like the end.mp3");
	audio::addToBgmList("music/kind of like the end.mp3");
	audio::loadSound("music/pp.flac");
	audio::addToBgmList("music/pp.flac");
	windowResize(s, wWidth, wHeight);
}

void StateGame::close(StateManager& s)
{
	saveSettings();

	save();
	if (s.getUiPage() != nullptr)
	{
		enableCursor(s.getWindow());
		s.setUiPage(nullptr);
	}
}

void StateGame::pause(StateManager& s)
{
}

void StateGame::resume(StateManager& s)
{
	if (paused)
	{
		s.setUiPage(&pauseMenu);
	}
}

void StateGame::update(StateManager& s, double dt)
{
	if (!scaleUIx2Checkbox.getChecked())
	{
		fpsText.setSize(2);
		coordsText.setSize(1);
		coordsText.setOffsetY(25);
	}
	else
	{
		fpsText.setSize(4);
		coordsText.setSize(2);
		coordsText.setOffsetY(45);
	}

	audio::updateBgm();

	float speed = playerSpeed * (keys.ctrl ? playerSprintMod : 1.0f);

	if (flight)
	{
		m5::vec5 moveDir{ 0 };

		if (keys.w) moveDir += cam.forward;
		if (keys.s) moveDir -= cam.forward;
		if (keys.a) moveDir += cam.left;
		if (keys.d) moveDir -= cam.left;
		if (keys.q) moveDir += cam.over;
		if (keys.e) moveDir -= cam.over;
		if (keys.r) moveDir += cam.yonder;
		if (keys.f) moveDir -= cam.yonder;

		moveDir = m5::normalize(moveDir);

		vel = moveDir * speed;

		if (keys.space) vel.a += speed;
		if (keys.shift) vel.a -= speed;
	}
	else
	{
		if (!onGround)
		{
			float g = gravity;

			// in water
			if (waterCheckbox.getChecked() && collision && cam.pos.a - playerHeight < 30.75f)
			{
				g *= 0.2f;
			}

			vel.a += g * dt;
			if (cam.pos.a < -10.0f)
			{
				vel.a = 0.0f;
				cam.pos.a = -10.0f;
				onGround = true;
			}
		}

		// a vector pointing outwards from the front of the player
		m5::vec5 pd = m5::normalize(m5::cross(cam.left, m5::vec5::up(), cam.over, cam.yonder));

		m5::vec5 moveDir{ 0 };

		if (keys.w) moveDir += pd;
		if (keys.s) moveDir -= pd;
		if (keys.a) moveDir += cam.left;
		if (keys.d) moveDir -= cam.left;
		if (keys.q) moveDir += cam.over;
		if (keys.e) moveDir -= cam.over;
		if (keys.r) moveDir += cam.yonder;
		if (keys.f) moveDir -= cam.yonder;

		moveDir = m5::normalize(moveDir);

		m5::vec5 targetVel = moveDir * speed;
		targetVel.a = vel.a;

		m5::vec5 velDiff = targetVel - vel;

		m5::vec5 movement = velDiff * 24.0f;

		vel += movement * dt;

		if (onGround && keys.space)
		{
			onGround = false;
			vel.a += 12.0f;
		}
	}

	if (collision)
	{
		double t = dt;
		for (int i = 0; i < 5 && t > 0.00000001; ++i)
		{
			float velLength = vel.length();
			if (velLength < 0.00000001f)
			{
				break;
			}

			m5::vec5 velNormalized = vel / velLength;

			velLength *= t;

			World::Collision closestHit{};
			closestHit.dist = 10000000.0f;
			float closestHitHeight = 0.0f;
			for (int j = 0; j <= 4; ++j)
			{
				float height = playerHeight * j / 4.0f;
				m5::vec5 pos = cam.pos;
				pos.a -= height;
				auto hit = world.dda(pos, velNormalized, velLength);
				if (hit.blockId && hit.dist < closestHit.dist)
				{
					closestHit = hit;
					closestHitHeight = height;
				}
			}

			if (closestHit.blockId)
			{
				//cam.pos = closestHit.pos + closestHit.normal * 0.001f;
				//cam.pos.a += closestHitHeight;

				float tHit = glm::max(closestHit.dist - 0.001f, 0.0f) / velLength * t;
				t -= tHit;
				cam.pos += vel * tHit;

				float vn = m5::dot(vel, closestHit.normal);
				if (vn < 0.0f)
				{
					vel -= closestHit.normal * vn;
					if (closestHit.side == 1)
					{
						onGround = true;
					}
				}
				cam.pos += closestHit.normal * 0.001f;
			}
			else
			{
				cam.pos += vel * t;
				break;
			}
		}

		m5::vec5 feetPos = cam.pos;
		feetPos.a -= playerHeight;
		feetPos.a += 0.01f;
		auto groundCheck = world.dda(feetPos, -m5::vec5::up(), 0.1f);
		if (!groundCheck.blockId)
		{
			onGround = false;
		}
	}
	else
	{
		cam.pos += vel * dt;
	}

	coordsText.setText(std::format(
		"Pos: A:{:+.1f} B:{:+.1f} C:{:+.1f} D:{:+.1f} E:{:+.1f}\n"
		"Up: A:{:+.1f} B:{:+.1f} C:{:+.1f} D:{:+.1f} E:{:+.1f}\n"
		"Forward: A:{:+.1f} B:{:+.1f} C:{:+.1f} D:{:+.1f} E:{:+.1f}\n"
		"Left: A:{:+.1f} B:{:+.1f} C:{:+.1f} D:{:+.1f} E:{:+.1f}\n"
		"Over: A:{:+.1f} B:{:+.1f} C:{:+.1f} D:{:+.1f} E:{:+.1f}\n"
		"Yonder: A:{:+.1f} B:{:+.1f} C:{:+.1f} D:{:+.1f} E:{:+.1f}\n",
		cam.pos.a, cam.pos.b, cam.pos.c, cam.pos.d, cam.pos.e,
		cam.up.a, cam.up.b, cam.up.c, cam.up.d, cam.up.e,
		cam.forward.a, cam.forward.b, cam.forward.c, cam.forward.d, cam.forward.e,
		cam.left.a, cam.left.b, cam.left.c, cam.left.d, cam.left.e,
		cam.over.a, cam.over.b, cam.over.c, cam.over.d, cam.over.e,
		cam.yonder.a, cam.yonder.b, cam.yonder.c, cam.yonder.d, cam.yonder.e
	));

	if (console.open)
	{
		if (glfwGetInputMode(s.getWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		{
			enableCursor(s.getWindow());
		}
		if (s.getUiPage() != &console.ui)
		{
			s.setUiPage(&console.ui);
		}
	}

	target = world.dda(cam.pos, cam.forward, 100.0);
}

void StateGame::render(StateManager& s)
{
	int wW, wH;
	s.getSize(&wW, &wH);

	static double lastTime = glfwGetTime();
	double curTime = glfwGetTime();
	double dt = curTime - lastTime;
	lastTime = curTime;

	double fps = 1.0 / dt;
	fpsText.setText(std::format("FPS: {:.1f}", fps));

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	cameraBuf.uploadData<Camera>(&cam);

	rendererShader->use();
	blockDataHandlesBuffer.use(1);
	blockTexturesBuffer.use(2);
	tileTextureHandles.use(3);
	rendererShader->setUniform("shadows", shadowsCheckbox.getChecked());
	rendererShader->setUniform("ambientOcclusion", ambientOcclusionCheckbox.getChecked());
	rendererShader->setUniform("water", waterCheckbox.getChecked());
	rendererShader->setUniform("lightDir", lightDir);
	rendererShader->setUniform("time", (float)glfwGetTime());
	rendererShader->setUniform("target.targeting", target.blockId != 0);
	rendererShader->setUniform("target.blockPos", target.blockPos);
	rendererShader->setUniform("target.side", target.side);
	QuadRendererBasic::render();

	glDisable(GL_DEPTH_TEST);
	ui.render();

	if (console.open)
	{
		console.qr.setMode(QuadRenderer::MODE_FILL);
		console.qr.setPos(0, 0, wW, console.height);
		console.qr.setColor({ 0.05f, 0.01f, 0.05f, 0.8f });
		console.qr.render();

		console.qr.setMode(QuadRenderer::MODE_LINES);
		console.qr.setPos(1, 1, wW - 1, console.height - 1);
		console.qr.setColor({ 1.0f, 1.0f, 1.0f, 0.8f });
		console.qr.render();

		int size = console.inputText.getSize();
		int x = 0, y = 0, w = 0, h = 0;
		console.inputText.getBounds(&console.ui, &x, &y, &w, &h);
		console.qr.setPos(x + 8 * (console.cursorPos + 2) * size, y, 7 * size, 8 * size);
		console.qr.render();

		x = 0; y = 0; w = 0; h = 0;
		console.logText.setWrapWidth(wW);
		console.logText.getBounds(&console.ui, &x, &y, &w, &h);
		console.logText.setOffsetY(console.height - 8 - 5 - h);
	}

	if (paused)
	{
		int wW, wH;
		s.getSize(&wW, &wH);

		qr.setMode(QuadRenderer::MODE_FILL);
		qr.setColors({
			glm::vec4{ 0.15f,	0.07f,	0.17f,	0.7f },
			glm::vec4{ 0.0f,	0.0f,	0.0f,	0.7f },
			glm::vec4{ 0.0f,	0.0f,	0.0f,	0.7f },
			glm::vec4{ 0.15f,	0.07f,	0.17f,	0.7f },
		});
		qr.setPos(0, 0, wW, wH);
		qr.render();
	}
}

void StateGame::mouseInput(StateManager& s, double xpos, double ypos)
{
	float dX = -(xpos - lastMousePos.x) * 2;
	float dY = (ypos - lastMousePos.y) * 2;

	lastMousePos = glm::vec2{ xpos, ypos };

	float hAngleDelta = dX / 1000.0f * glm::pi<float>();
	float vAngleDelta = dY / 1000.0f * glm::pi<float>();

	// a vector pointing outwards from the front of the player
	m5::vec5 pd = m5::normalize(m5::cross(cam.left, m5::vec5::up(), cam.over, cam.yonder));

	if (!keys.mmb)
	{
		float v = glm::asin(glm::clamp(-cam.forward.a, -1.0f, 1.0f));
		vAngleDelta = glm::clamp(vAngleDelta + v, -glm::half_pi<float>() + 0.001f, glm::half_pi<float>() - 0.001f) - v;

		m5::rotor5 rotH{ m5::wedge(pd, cam.left), hAngleDelta }; // BC
		m5::rotor5 rotV{ m5::wedge({ 1, 0, 0, 0, 0 }, pd), vAngleDelta }; // AB

		orientation = rotH * rotV * orientation;
	}
	else
	{
		if (!keys.alt)
		{
			m5::rotor5 rotH{ m5::wedge(cam.over, cam.left), hAngleDelta }; // CD
			m5::rotor5 rotV{ m5::wedge(pd, cam.over), vAngleDelta }; // BD

			orientation = rotH * rotV * orientation;
		}
		else
		{
			m5::rotor5 rotH{ m5::wedge(cam.yonder, cam.left), hAngleDelta }; // CE
			m5::rotor5 rotV{ m5::wedge(pd, cam.yonder), vAngleDelta }; // BE

			orientation = rotH * rotV * orientation;
		}
	}

	updateCamDirs();
}

void StateGame::charInput(StateManager& s, uint32_t codepoint)
{
	if (console.open)
	{
		if (console.blockCharInput)
		{
			console.blockCharInput = false;
			return;
		}

		if (codepoint >= 1 && codepoint <= 127)
		{
			console.input.insert(console.cursorPos++, 1, (char)codepoint);
		}

		updateConsoleInput();
	}

}

void StateGame::updateConsoleInput()
{
	console.inputText.setText("> " + (console.input.empty() ? "Enter a command. (\"help\" for a list of commands)" : console.input));
	console.inputText.setColor(console.input.empty() ? glm::vec4{ 0.65f, 0.65f, 0.65f, 1.0f } : glm::vec4{ 1.0f });
}

void StateGame::scrollInput(StateManager& s, double xoff, double yoff)
{
	if (keys.alt)
	{
		playerSpeed += yoff * 0.2;
		playerSpeed = glm::max(playerSpeed, 0.0f);
	}
	else
	{
		if (yoff < 0)
		{
			selectBlock(selectedBlock < Block::COUNT - 1 ? selectedBlock + 1 : Block::AIR + 1);
		}
		else if (yoff > 0)
		{
			selectBlock(selectedBlock > Block::AIR + 1 ? selectedBlock - 1 : Block::COUNT - 1);
		}
	}
}

void StateGame::mouseButtonInput(StateManager& s, int button, int action, int mods)
{
	if (action == GLFW_REPEAT) return;

	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		keys.lmb = (action == GLFW_PRESS);
		if (keys.lmb)
		{
			world.setBlock(target.blockPos, Block::AIR);
			updateRendererData();
		}
	} break;
	case GLFW_MOUSE_BUTTON_MIDDLE: keys.mmb = (action == GLFW_PRESS); break;
	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		keys.rmb = (action == GLFW_PRESS);
		if (keys.rmb)
		{
			world.setBlock(target.blockPos + target.normal, selectedBlock);
			updateRendererData();
		}
	} break;
	}
}

void StateGame::keyInput(StateManager& s, int key, int scancode, int action, int mods)
{
	if (console.open)
	{
		if (action == GLFW_RELEASE) return;

		switch (key)
		{
		case GLFW_KEY_BACKSPACE:
		{
			if (mods & GLFW_MOD_CONTROL)
			{
				size_t next = console.input.rfind(' ', console.cursorPos);
				if (next == std::string::npos) next = 0;
				console.input.erase(next, console.cursorPos - next);
				console.cursorPos = next;
			}
			else
			{
				console.cursorPos = glm::clamp(console.cursorPos - 1, 0, (int)console.input.size());
				console.input.erase(console.cursorPos, 1);
			}
		} break;
		case GLFW_KEY_DELETE:
		{
			if (mods & GLFW_MOD_CONTROL)
			{
				size_t next = console.input.find_first_of(' ', glm::clamp(console.cursorPos + 1, 0, (int)console.input.size()));
				if (next == std::string::npos) next = console.input.size();
				console.input.erase(console.cursorPos, next - console.cursorPos);
			}
			else
			{
				console.input.erase(console.cursorPos, 1);
			}
		} break;
		case GLFW_KEY_LEFT:
		{
			console.cursorPos = glm::clamp(console.cursorPos - 1, 0, (int)console.input.size());
			if (mods & GLFW_MOD_CONTROL)
			{
				size_t next = console.input.rfind(' ', console.cursorPos);
				if (next == std::string::npos) next = 0;
				console.cursorPos = next;
			}
		} break;
		case GLFW_KEY_RIGHT:
		{
			console.cursorPos = glm::clamp(console.cursorPos + 1, 0, (int)console.input.size());
			if (mods & GLFW_MOD_CONTROL)
			{
				size_t next = console.input.find_first_of(' ', console.cursorPos);
				if (next == std::string::npos) next = console.input.size();
				console.cursorPos = next;
			}
		} break;
		case GLFW_KEY_HOME:
		{
			console.cursorPos = 0;
		} break;
		case GLFW_KEY_END:
		{
			console.cursorPos = console.input.size();
		} break;
		case GLFW_KEY_ESCAPE:
		{
			if (action == GLFW_PRESS)
			{
				console.open = false;
				disableCursor(s.getWindow());
				s.setUiPage(nullptr);
			}
		} break;
		case GLFW_KEY_ENTER:
		{
			exec(console.input);
			console.input = "";
			console.cursorPos = 0;
		} break;
		case GLFW_KEY_UP:
		{
			if (console.historyCount > 0)
			{
				console.input = console.history[console.historyIndex];
				console.cursorPos = console.input.size();

				console.historyIndex = (console.historyIndex + 1) % console.historyCount;
			}
		} break;
		case GLFW_KEY_DOWN:
		{
			if (console.historyIndex > 0)
			{
				console.historyIndex = (console.historyIndex - 1) % console.historyCount;

				console.input = console.history[console.historyIndex];
				console.cursorPos = console.input.size();
			}
		} break;
		}
		updateConsoleInput();
		return;
	}

	if (action == GLFW_REPEAT) return;

	switch (key)
	{
	//case GLFW_KEY_0:
	case GLFW_KEY_1:
	case GLFW_KEY_2:
	case GLFW_KEY_3:
	case GLFW_KEY_4:
	case GLFW_KEY_5:
	case GLFW_KEY_6:
	{
		if (action == GLFW_PRESS)
		{
			selectBlock(key - GLFW_KEY_0);
		}
	} break;
	case GLFW_KEY_V:
	{
		if (action == GLFW_PRESS)
		{
			flight = !flight;
		}
	} break;
	case GLFW_KEY_C:
	{
		if (action == GLFW_PRESS)
		{
			collision = !collision;
		}
	} break;
	case GLFW_KEY_W: keys.w = (action == GLFW_PRESS); break;
	case GLFW_KEY_S:
	{
		keys.s = (action == GLFW_PRESS);
		if (keys.s && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT))
		{
			save();
		}
	} break;
	case GLFW_KEY_A: keys.a = (action == GLFW_PRESS); break;
	case GLFW_KEY_D: keys.d = (action == GLFW_PRESS); break;
	case GLFW_KEY_Q: keys.q = (action == GLFW_PRESS); break;
	case GLFW_KEY_E: keys.e = (action == GLFW_PRESS); break;
	case GLFW_KEY_R: keys.r = (action == GLFW_PRESS); break;
	case GLFW_KEY_F: keys.f = (action == GLFW_PRESS); break;
	case GLFW_KEY_SPACE: keys.space = (action == GLFW_PRESS); break;
	case GLFW_KEY_LEFT_SHIFT: keys.shift = (action == GLFW_PRESS); break;
	case GLFW_KEY_LEFT_CONTROL: keys.ctrl = (action == GLFW_PRESS); break;
	case GLFW_KEY_LEFT_ALT: keys.alt = (action == GLFW_PRESS); break;
	case GLFW_KEY_ESCAPE:
	{
		if (action == GLFW_PRESS)
		{
			console.open = false;
			if (s.getUiPage() == nullptr)
			{
				enableCursor(s.getWindow());
				s.setUiPage(&pauseMenu);
				paused = true;
			}
			else
			{
				disableCursor(s.getWindow());
				s.setUiPage(nullptr);
				paused = false;
			}
		}
	} break;
	case GLFW_KEY_GRAVE_ACCENT:
	{
		if (action == GLFW_PRESS)
		{
			console.open = !console.open;

			if (console.open)
			{
				enableCursor(s.getWindow());
				s.setUiPage(&console.ui);
				console.blockCharInput = true;
			}
			else
			{
				disableCursor(s.getWindow());
				s.setUiPage(nullptr);
			}
		}
	} break;
	}
}

std::optional<std::string> StateGame::strArg(const std::vector<std::string>& args, int& cursor)
{
	if (cursor + 1 > args.size())
	{
		return std::nullopt;
	}

	std::string result = args[cursor];

	cursor += 1;

	return result;
}

std::optional<int64_t> StateGame::intArg(const std::vector<std::string>& args, int& cursor, int64_t min, int64_t max)
{
	if (cursor + 1 > args.size())
	{
		return std::nullopt;
	}

	int64_t result;

	try
	{
		const std::string& arg = args[cursor];
		int64_t x = std::stoll(arg);
		result = x;
	}
	catch (const std::exception&)
	{
		return std::nullopt;
	}

	if (result < min || result > max)
	{
		return std::nullopt;
	}

	cursor += 1;

	return result;
}

std::optional<double> StateGame::floatArg(const std::vector<std::string>& args, int& cursor, double min, double max)
{
	if (cursor + 1 > args.size())
	{
		return std::nullopt;
	}

	double result;

	try
	{
		const std::string& arg = args[cursor];
		double x = std::stod(arg);
		result = x;
	}
	catch (const std::exception&)
	{
		return std::nullopt;
	}

	if (result < min || result > max)
	{
		return std::nullopt;
	}

	cursor += 1;

	return result;
}

std::optional<m5::vec5> StateGame::posArg(const std::vector<std::string>& args, int& cursor)
{
	if (cursor + 5 > args.size())
	{
		return std::nullopt;
	}

	m5::vec5 result{};

	try
	{
		for (int i = 0; i < 5; ++i)
		{
			const std::string& arg = args[cursor + i];
			if (arg[0] == '~')
			{
				result[i] = cam.pos[i];

				if (arg.size() > 1)
				{
					if (arg.size() > 2 && (arg[1] == '+' || arg[1] == '-'))
					{
						try
						{
							float x = std::stof(arg.substr(2));
							result[i] += x * (arg[1] == '-' ? -1 : 1);
						}
						catch (const std::exception&)
						{
							return std::nullopt;
						}
					}
					else
					{
						try
						{
							float x = std::stof(arg.substr(1));
							result[i] += x;
						}
						catch (const std::exception&)
						{
							return std::nullopt;
						}
					}
				}
			}
			else
			{
				try
				{
					float x = std::stof(arg);
					result[i] = x;
				}
				catch (const std::exception&)
				{
					return std::nullopt;
				}
			}
		}
	}
	catch (const std::exception&)
	{
		return std::nullopt;
	}

	cursor += 5;

	return result;
}

void StateGame::exec(std::string_view cmd)
{
	std::string c = std::string{ cmd };
	utils::trim(c);

	if (c.empty())
	{
		return;
	}

	{
		auto history = console.history;
		std::move(history.begin(), history.end() - 1, console.history.begin() + 1);
	}

	console.history[0] = c;
	console.historyIndex = 0;
	console.historyCount = glm::min(console.historyCount + 1, (int)console.history.size());

	auto args = utils::splitStr(c, ' ');
	std::remove_if(args.begin(), args.end(), [](const std::string& str) { return str.empty(); });

	// switch(string) at home:
	std::unordered_map<std::string, std::function<bool()>> commands
	{
		{ "help", [&]()
		{
			print(
				"- help - Outputs this list.\n"
				"- clear - Clears the log.\n"
				"- setBlock <pos> <id> - Sets a block at a position, e.g. \"setBlock ~ ~1 ~+1 ~-1 ~ 3\".\n"
				"- fill <posA> <posB> <id> - Fills a region with a block.\n"
				"- tp <pos> - Teleports the player to a position, e.g. \"tp 0 32 0 0 0\".\n"
				"- align <compA> <compB> - Aligns the player to the plane \"A<compA><compB>\", e.g. \"align BC\".\n"
				"- save - Saves the world.\n"
				"- speed [ups] - Gets/Sets the movement speed. Default = 7. e.g. \"speed 0.1\""
				, false
			);
			return true;
		}},
		{ "clear", [&]()
		{
			console.log.clear();
			console.logText.setText(console.log);
			return true;
		}},
		{ "setBlock", [&]()
		{
			int cursor = 1;

			auto pos = posArg(args, cursor);
			if (!pos.has_value())
			{
				return false;
			}
			auto id = intArg(args, cursor, 0, Block::COUNT - 1);
			if (!id.has_value())
			{
				return false;
			}

			world.setBlock(pos.value(), id.value());
			updateRendererData();

			return true;
		}},
		{ "fill", [&]()
		{
			int cursor = 1;

			auto posA = posArg(args, cursor);
			if (!posA.has_value())
			{
				return false;
			}
			auto posB = posArg(args, cursor);
			if (!posB.has_value())
			{
				return false;
			}
			auto id = intArg(args, cursor, 0, Block::COUNT - 1);
			if (!id.has_value())
			{
				return false;
			}

			int minA = glm::min(posA.value().a, posB.value().a); int maxA = glm::max(posA.value().a, posB.value().a);
			int minB = glm::min(posA.value().b, posB.value().b); int maxB = glm::max(posA.value().b, posB.value().b);
			int minC = glm::min(posA.value().c, posB.value().c); int maxC = glm::max(posA.value().c, posB.value().c);
			int minD = glm::min(posA.value().d, posB.value().d); int maxD = glm::max(posA.value().d, posB.value().d);
			int minE = glm::min(posA.value().e, posB.value().e); int maxE = glm::max(posA.value().e, posB.value().e);
			for (int e = minE; e <= maxE; ++e)
				for (int d = minD; d <= maxD; ++d)
					for (int c = minC; c <= maxC; ++c)
						for (int b = minB; b <= maxB; ++b)
							for (int a = minA; a <= maxA; ++a)
							{
								world.setBlock({ a,b,c,d,e }, id.value());
							}
			updateRendererData();

			print(std::format("Filled {} blocks.", (maxA - minA + 1) * (maxB - minB + 1) * (maxC - minC + 1) * (maxD - minD + 1) * (maxE - minE + 1)));

			return true;
		}},
		{ "tp", [&]()
		{
			int cursor = 1;

			auto pos = posArg(args, cursor);
			if (!pos.has_value())
			{
				return false;
			}

			cam.pos = pos.value();

			return true;
		}},
		{ "align", [&]()
		{
			int cursor = 1;

			// get the plane string as one or two arguments
			std::string s;
			do
			{
				auto arg = strArg(args, cursor);
				if (!arg.has_value())
				{
					// fewer than two chars parsed
					return false;
				}

				s += arg.value();

				if (s.length() > 2)
				{
					// more than two chars parsed
					return false;
				}
			} while (s.length() < 2);

			// to lower case
			utils::toLower(s);

			if (s[0] == s[1] ||
				s[0] < 'b' || s[0] > 'e' ||
				s[1] < 'b' || s[1] > 'e')
			{
				return false;
			}

			int compA = s[0] - 'a';
			int compB = s[1] - 'a';

			if (compA > compB)
			{
				std::swap(compA, compB);
			}

			orientation = {};

			if (compA != 1)
			{
				m5::vec5 srcA{ 0, 1, 0, 0, 0 };
				m5::vec5 dstA{ 0.0f };
				dstA[compA] = 1.0f;
				orientation = m5::rotor5{ srcA, dstA } * orientation;
			}

			if (compB != 2)
			{
				m5::vec5 srcB = orientation.rotate({ 0, 0, 1, 0, 0 });
				m5::vec5 dstB{ 0.0f };
				dstB[compB] = 1.0f;
				orientation = m5::rotor5{ srcB, dstB } * orientation;
			}

			updateCamDirs();

			return true;
		}},
		{ "save", [&]()
		{
			save();

			return true;
		}},
		{ "speed", [&]()
		{
			int cursor = 1;

			auto speed = floatArg(args, cursor, 0.0);
			if (!speed.has_value())
			{
				print(std::format("The movement speed is {:.3f} u/s.", playerSpeed));

				return true;
			}

			playerSpeed = speed.value();

			print(std::format("Set the movement speed to {:.3f} u/s.", playerSpeed));

			return true;
		} },
	};

	print(std::format("{}", cmd), true);

	if (commands.contains(args[0]))
	{
		if (!commands[args[0]]())
		{
			print(std::format("Failed to execute \"{}\".", cmd));
		}
		return;
	}

	print(std::format("Unknown command \"{}\"! Use \"help\" for a list of commands.", cmd));
}

void StateGame::print(std::string_view message, bool includeTime)
{
	console.log += '\n';
	if (includeTime)
	{
		auto time = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
		console.log += std::format("[{:%H:%M:%S}]: ", time);
	}
	console.log += message;

	console.logText.setText(console.log);
}

void StateGame::windowResize(StateManager&, int width, int height)
{
	float w = width;
	float h = height;
	float invW = 1.0f / w;
	float invH = 1.0f / h;
	projection3D = glm::perspective(cam.vFov, w * invH, 0.03f, 1000.0f);

	rendererShader->setUniform("screenSize", w, h, invW, invH);

	ui.windowResize(width, height);
}

World* StateGame::createWorld(uint8_t edgeLength)
{
	world = World{ edgeLength };
	return &world;
}

void StateGame::setWorldPath(std::filesystem::path path)
{
	worldPath = path;
}

void StateGame::updateCamDirs()
{
	// it's probably faster to convert to a matrix to get the vectors
	cam.up = m5::normalize(orientation.rotate(m5::vec5::up()));
	cam.forward = m5::normalize(orientation.rotate(m5::vec5::forward()));
	cam.left = m5::normalize(orientation.rotate(m5::vec5::left()));
	cam.over = m5::normalize(orientation.rotate(m5::vec5::over()));
	cam.yonder = m5::normalize(orientation.rotate(m5::vec5::yonder()));
}

void StateGame::enableCursor(GLFWwindow* window)
{
	utils::setCursorPos(window, lastMousePos.x, lastMousePos.y);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	utils::showCursor(window);
}

void StateGame::disableCursor(GLFWwindow* window)
{
	utils::getCursorPos(window, &lastMousePos.x, &lastMousePos.y);

	//utils::setCursorPos(window, 0.0, 0.0);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	utils::hideCursor(window);
}

void StateGame::save()
{
	std::ofstream{ worldPath / "player.json" } << nlohmann::json{
		{ "pos", cam.pos.toJson() },
		{ "orientation", orientation.toJson() },
	};

	std::ofstream{ worldPath / "data.bin", std::ios::binary }.write(
		reinterpret_cast<const char*>(world.getChunks()),
		world.getDataSize()
	);

	print("Saved the world.");
}

void StateGame::selectBlock(uint8_t blockId)
{
	selectedBlock = blockId;
	selectedBlockText.setText("Selected Block: " + std::string{ Block::names[blockId] });
	selectedBlockImage.setClip((float)(blockId - 1) / 16.0f, 0, 1.0f / 16.0f, 1.0f);
}

constexpr uint8_t SECTIONS = 4u; // RGBA channels
constexpr uint8_t SECTION_HEIGHT = Chunk::HEIGHT / SECTIONS;
constexpr uint8_t BLOCK_BITS = 4u;
constexpr uint8_t BLOCKS_VERTICAL = 32u / BLOCK_BITS;
constexpr uint32_t CHUNK_BLOCKS = SECTION_HEIGHT * Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * Chunk::SIZE / BLOCKS_VERTICAL;
using BlockArray = std::array<uint32_t, CHUNK_BLOCKS>;

void StateGame::updateRendererData()
{
	glm::ivec4 minC{ 0 }, maxC{ (int)world.getEdgeLength() - 1 };
	glm::ivec4 size = maxC - minC + 1;

	uint32_t sectionSize = size.x * size.y * size.z * size.w;
	uint32_t* chunks = new uint32_t[sectionSize * SECTIONS](0);

	int tex = 0;
	for (int b = 0; b < size.x; ++b)
	for (int c = 0; c < size.y; ++c)
	for (int d = 0; d < size.z; ++d)
	for (int e = 0; e < size.w; ++e)
	{
		glm::ivec4 pos{ b,c,d,e };
		Chunk* chunk = world.getChunk(pos);

		if (!chunk)
		{
			continue;
		}

		glm::uvec4 uPos = pos - minC;

		uint32_t ind = uPos.x + uPos.y * size.x + uPos.z * size.x * size.y + uPos.w * size.x * size.y * size.z;

		for (int s = 0; s < SECTIONS; ++s)
		{
			BlockArray blocks{ 0 };

			bool empty = true;
			if (!chunk->shouldUpdateRenderer[s])
			{
				chunks[ind * SECTIONS + s] = chunk->rendererHandleIndices[s];
				continue;
			}
			for (int ab = 0; ab < Chunk::SIZE; ++ab)
				for (int ac = 0; ac < Chunk::SIZE; ++ac)
					for (int ad = 0; ad < Chunk::SIZE; ++ad)
						for (int ae = 0; ae < Chunk::SIZE; ++ae)
						{
							uint32_t packed[4]{ 0 };
							std::memcpy(packed, &chunk->blocks[ab][ac][ad][ae].blockData[s * SECTION_HEIGHT / 2], sizeof(packed));

							if (packed[0] || packed[1] || packed[2] || packed[3])
							{
								empty = false;

								uint32_t texX = ab;
								uint32_t texY = ac;
								uint32_t texZ = ad + ae * Chunk::SIZE;

								uint32_t indB = texX + texY * Chunk::SIZE + texZ * Chunk::SIZE * Chunk::SIZE;

								std::memcpy(&blocks[indB * 4], packed, sizeof(packed));
							}
						}

			if (!empty)
			{
				if (chunk->rendererTexIndices[s] && chunk->rendererHandleIndices[s] && chunk->rendererTexIndices[s] <= blockDataBuffers.size() && chunk->rendererHandleIndices[s] <= blockDataHandles.size())
				{
					blockDataBuffers[chunk->rendererTexIndices[s] - 1].uploadData(Chunk::SIZE, Chunk::SIZE, Chunk::SIZE * Chunk::SIZE, blocks.data());
					blockDataHandles[chunk->rendererHandleIndices[s] - 1] = blockDataBuffers[chunk->rendererTexIndices[s] - 1].getHandle();
					//printf("updated %llu:%llu\n", chunk->rendererTexIndices[s] - 1, chunk->rendererHandleIndices[s] - 1);
				}
				else
				{
					blockDataBuffers.emplace_back(TextureBuffer{ Chunk::SIZE, Chunk::SIZE, Chunk::SIZE * Chunk::SIZE, TextureFormat::RGBA32u, blocks.data() });
					blockDataHandles.emplace_back(blockDataBuffers.back().getHandle());
					chunk->rendererTexIndices[s] = blockDataBuffers.size(); // index + 1
					chunk->rendererHandleIndices[s] = blockDataHandles.size(); // index + 1
					//printf("added %llu:%llu\n", chunk->rendererTexIndices[s] - 1, chunk->rendererHandleIndices[s] - 1);
				}
			}
			else
			{
				chunk->rendererTexIndices[s] = 0;
				chunk->rendererHandleIndices[s] = 0;
			}

			chunks[ind * SECTIONS + s] = chunk->rendererHandleIndices[s];
			chunk->shouldUpdateRenderer[s] = false;
		}
	}

	blockDataHandlesBuffer.uploadData(0, sizeof(uint64_t) * blockDataHandles.size(), blockDataHandles.data());
	chunksBuffer.uploadData(size.x, size.y, size.z * size.w, chunks);

	delete[] chunks;

	chunks = nullptr;

	{
		rendererShader->setUniform("chunksMinBound", minC);
		rendererShader->setUniform("chunksSize", size);
		rendererShader->setUniform("chunks", chunksBuffer.getHandle());
	}
}
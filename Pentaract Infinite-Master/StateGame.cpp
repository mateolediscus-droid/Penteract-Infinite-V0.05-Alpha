#include "StateGame.h"
#include "BackgroundWork.h"
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
#include "SideView.h"
#include "HyperplaneAlignment.h"
#include "WorldRenderType.h"

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
		{ "inventoryExcluded", inventoryExcluded },
	};

	file << j.dump(1, '\t');

	file.close();
}

bool StateGame::playerIntersectsBlock(const m5::vec5& pos, bool* pending)
{
	if (pending) *pending = false;
	const int b = (int)glm::floor(pos.b);
	const int c = (int)glm::floor(pos.c);
	const int d = (int)glm::floor(pos.d);
	const int e = (int)glm::floor(pos.e);

	for (int i = 0; i <= 4; ++i)
	{
		float height = playerHeight * i / 4.0f;
		int a = (int)glm::floor(pos.a - height);
		m5::ivec5 block{a,b,c,d,e};
		const m5::ivec5 extent{32,8,8,8,8};
		m5::ivec5 key = m5::floor(m5::vec5(block) / m5::vec5(extent));
		auto chunk = world.findChunk(key);
		if (!chunk) {
			if (world.canGenerateChunk(key)) {world.requestChunk(key);if(pending)*pending=true;return false;}
			continue;
		}
		if (ExtendedBlock::isSolid(chunk->getBlockFull(m5::uvec5(block-key*extent))))
		{
			return true;
		}
	}

	return false;
}

void StateGame::movePlayerOutOfBlocks()
{
	bool pending = false;
	if (!playerIntersectsBlock(cam.pos, &pending))
	{
		return;
	}

	m5::vec5 candidate = cam.pos;
	float startA = glm::floor(cam.pos.a);
	float fallbackA = glm::max(
		startA + (float)Chunk::HEIGHT * 4.0f,
		(float)world.getNaturalTerrainMaxA() + (float)Chunk::HEIGHT + playerHeight
	);

	for (float a = startA; a <= fallbackA; a += 0.25f)
	{
		candidate.a = a;
		bool occupied = playerIntersectsBlock(candidate, &pending);
		if (pending) return;
		if (!occupied)
		{
			cam.pos = candidate;
			vel = {};
			onGround = false;
			return;
		}
	}

	cam.pos.a = fallbackA;
	vel = {};
	onGround = false;
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
		inventoryExcluded = j.value("inventoryExcluded", std::vector<uint16_t>{});
		if (inventoryBlocks(true).empty()) inventoryExcluded.clear();
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
	sideWalkTime = 0.0;
	sideWalkDirection = 0;
	if (renderType == 2)
	{
		for (const auto* frame : {"idle", "walk1", "walk2-left", "walk2-right", "jump"})
			if (!Texture::get(std::format("player2d.{}", frame)) && !Texture::load(std::format("assets/textures/player2d/{}.png", frame), 4, false,
				std::format("player2d.{}", frame)))
				throw std::runtime_error(std::format("Missing player sprite: {}", frame));
	}
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
		Texture::load("assets/textures/deepstoneIcons.png", 4, false, "extended.deepstoneIcon", true);
		Texture::load("assets/textures/antiSandIcons.png", 4, false, "extended.antiSandIcon", true);
		Texture::load("assets/textures/leverIcons.png", 4, false, "extended.leverIcon", true);
		Texture::load("assets/textures/cableIcons.png", 4, false, "extended.cableIcon", true);
		Texture::load("assets/textures/crystalDoorIcons.png", 4, false, "extended.crystalDoorIcon", true);
		Texture::load("assets/textures/reapeaterIcons.png", 4, false, "extended.repeaterIcon", true);
		Texture::load("assets/textures/redirectorIcons.png", 4, false, "extended.redirectorIcon", true);
		Texture::load("assets/textures/StructureBlockIcon.png", 4, false, "extended.structureIcon", true);
		Texture::load("assets/textures/leafIcon.png", 4, false, "extended.leafIcon", true);
		selectedBlockImage.setScale(2, 2);
		selectedBlockImage.setTexture(Texture::get("blockIcons.png"));
		selectedBlockImage.setAlignX(ui::ALIGN_RIGHT);
		selectedBlockImage.setOffsetX(-50);
		selectedBlockImage.setAlignY(ui::ALIGN_BOTTOM);
		selectedBlockImage.setOffsetY(-50);

		selectBlock(inventoryBlocks(true).front());

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
		auto infoPath = worldPath / "info.json";
		auto info = std::filesystem::exists(infoPath) ? nlohmann::json::parse(std::ifstream(infoPath)) : nlohmann::json::object();
		int temporalDimensions = info.value("temporalDimensions", std::filesystem::exists(worldPath / "temporal.bin") ? 2 : 1);
		if (temporalDimensions == 2) world.enableTemporal(worldPath / "temporal.bin");
	}
	{
		std::filesystem::path playerPath = worldPath / "player.json";
		if (std::filesystem::is_regular_file(playerPath))
		{
			nlohmann::json j;
			std::ifstream{ playerPath } >> j;

			cam.pos = j["pos"];
			orientation = j["orientation"];
			sideAngle = j.value("sideAngle", 0.0f);
			if (!std::isfinite(sideAngle) || world.getDimensions() == 2) sideAngle = 0.0f;
			if (j.contains("sideBasis")) {
				auto saved = j.at("sideBasis").get<SideViewND::Basis<5>>();
				if (SideViewND::valid<5>(saved, world.getDimensions())) sideBasis = saved;
			} else if (world.getDimensions() >= 3) {
				auto right=SideView::right(sideAngle);
				sideBasis[0]={0.,right.b,right.c,0.,0.};sideBasis[1]={0.,-right.c,right.b,0.,0.};
			}
			auto plane = j.value("renderPlane", std::array<int, 2>{1, 2});
			if (plane[0] >= 1 && plane[0] < world.getDimensions() && plane[1] > plane[0] && plane[1] < world.getDimensions())
				renderPlaneComps = {plane[0], plane[1]};
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
		for (int axis = world.getDimensions(); axis < 5; ++axis) cam.pos[axis] = 0.5f;
		movePlayerOutOfBlocks();
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
	blockDataBuffers.clear();
	blockDataHandles.clear();
	chunksBuffer = TextureBuffer{ 1, 1, 1, TextureFormat::R32u };
	blockDataHandlesBuffer = GPUBuffer{ sizeof(uint64_t) };
	constexpr int TEXTURES_PER_BLOCK = 10;
	std::vector<int> allTextures(TEXTURES_PER_BLOCK * glm::max((int)ExtendedBlock::COUNT, (int)Block::EXTENDED_MARKER + 1), 0);
	for (int i = 0; i < Block::COUNT; ++i)
	{
		std::memcpy(allTextures.data() + i * TEXTURES_PER_BLOCK, Block::blocks[i].textures, sizeof(Block::blocks[i].textures));
	}
	for (int i = ExtendedBlock::FIRST; i < ExtendedBlock::COUNT; ++i)
	{
		std::memcpy(allTextures.data() + i * TEXTURES_PER_BLOCK, ExtendedBlock::data(i).textures, sizeof(ExtendedBlock::data(i).textures));
	}
	blockTexturesBuffer = GPUBuffer{ sizeof(int) * allTextures.size(), allTextures.data() };

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
	Texture::load("assets/textures/blocks/custom.ndtf", 4, false, "blocks.custom", true); // 9
	Texture::load("assets/textures/blocks/deepstone.ndtf", 4, false, "blocks.deepstone", true); // 10
	Texture::load("assets/textures/blocks/antiSand.ndtf", 4, false, "blocks.antiSand", true); // 11
	Texture::load("assets/textures/blocks/leverOff.ndtf", 4, false, "blocks.leverOff", true); // 12
	Texture::load("assets/textures/blocks/leverOn.ndtf", 4, false, "blocks.leverOn", true); // 13
	for (int i = 0; i < 16; ++i)
	{
		Texture::load(std::format("assets/textures/blocks/cable{}.ndtf", i), 4, false, std::format("blocks.cable{}", i), true); // 14-29
	}
	Texture::load("assets/textures/blocks/crystalDoor.ndtf", 4, false, "blocks.crystalDoor", true); // 30
	Texture::load("assets/textures/blocks/crystalDoorOpen.ndtf", 4, false, "blocks.crystalDoorOpen", true); // 31
	for (int i = 0; i < 16; ++i)
	{
		Texture::load(std::format("assets/textures/blocks/repeater{}.ndtf", i), 4, false, std::format("blocks.repeater{}", i), true); // 32-47
	}
	Texture::load("assets/textures/blocks/redirectorOff.ndtf", 4, false, "blocks.redirectorOff", true); // 48
	Texture::load("assets/textures/blocks/redirectorOn.ndtf", 4, false, "blocks.redirectorOn", true); // 49
	textures.emplace_back(Texture::get("blocks.dirt")->getHandle());
	textures.emplace_back(Texture::get("blocks.grassSide")->getHandle());
	textures.emplace_back(Texture::get("blocks.grassTop")->getHandle());
	textures.emplace_back(Texture::get("blocks.stone")->getHandle());
	textures.emplace_back(Texture::get("blocks.woodSide")->getHandle());
	textures.emplace_back(Texture::get("blocks.woodTop")->getHandle());
	textures.emplace_back(Texture::get("blocks.glass")->getHandle());
	textures.emplace_back(Texture::get("blocks.bricks")->getHandle());
	textures.emplace_back(Texture::get("blocks.water")->getHandle());
	textures.emplace_back(Texture::get("blocks.custom")->getHandle());
	textures.emplace_back(Texture::get("blocks.deepstone")->getHandle());
	textures.emplace_back(Texture::get("blocks.antiSand")->getHandle());
	textures.emplace_back(Texture::get("blocks.leverOff")->getHandle());
	textures.emplace_back(Texture::get("blocks.leverOn")->getHandle());
	for (int i = 0; i < 16; ++i)
	{
		textures.emplace_back(Texture::get(std::format("blocks.cable{}", i))->getHandle());
	}
	textures.emplace_back(Texture::get("blocks.crystalDoor")->getHandle());
	textures.emplace_back(Texture::get("blocks.crystalDoorOpen")->getHandle());
	for (int i = 0; i < 16; ++i)
	{
		textures.emplace_back(Texture::get(std::format("blocks.repeater{}", i))->getHandle());
	}
	textures.emplace_back(Texture::get("blocks.redirectorOff")->getHandle());
	textures.emplace_back(Texture::get("blocks.redirectorOn")->getHandle());
	for (int side = 0; side < 10; ++side)
	{
		auto name = std::format("blocks.leaf{}", side);
		Texture::load(std::format("assets/textures/blocks/leaf{}.ndtf", side), 4, false, name, true);
		textures.emplace_back(Texture::get(name)->getHandle());
	}
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
	if (structureSaveResult.valid())
	{
		try { structureSaveResult.get(); } catch (const std::exception& error) { print(error.what()); }
	}
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
	const auto previousPosition = cam.pos;
	rendererDirty |= world.collectGeneratedChunks(2);
	world.updateStructureRegions();
	pollStructureLibrary(dt);
	if (structureEditorOpen)
	{
		updateStructureEditor();
		return;
	}
	if (inventoryOpen) { if (inventoryRefresh) showInventory(s); return; }
	// Warm the player's immediate path, including the two hidden axes.
	// Collision queries keep a synchronous fallback for teleports/unusually fast travel.
	for (float ahead : { 0.0f, 0.5f })
	{
		m5::vec5 predicted = cam.pos + vel * ahead;
		for (float height : { 0.0f, playerHeight })
		{
			m5::vec5 point = predicted;
			point.a -= height;
			auto request = [this](const m5::vec5& p)
			{
				world.requestChunk({
					(int)glm::floor(p.a / (float)Chunk::HEIGHT),
					(int)glm::floor(p.b / (float)Chunk::SIZE),
					(int)glm::floor(p.c / (float)Chunk::SIZE),
					(int)glm::floor(p.d / (float)Chunk::SIZE),
					(int)glm::floor(p.e / (float)Chunk::SIZE) });
			};
			request(point);
			for (int axis = 0; axis < world.getDimensions(); ++axis)
			for (int dir : { -1, 1 })
			{
				auto neighbor = point;
				neighbor[axis] += dir * 2.0f;
				// Preserve the existing A- streaming threshold at A = 8.
				if (axis == 0 && cam.pos.a >= 8.0f && neighbor.a < 0.0f) continue;
				request(neighbor);
			}
		}
	}
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

	if (world.hasTemporal())
	{
		if (world.advanceTemporal(dt)) updateRendererData();
	}
	else
	{
	sandFallTimer += dt;
	if (sandFallTimer >= 0.5)
	{
		sandFallTimer = glm::mod(sandFallTimer, 0.5);
		if (world.updateFallingSand())
		{
			updateRendererData();
		}
	}

	if (world.hasActiveRedstoneTicks())
	{
		redstoneCheckTimer += dt;
		if (redstoneCheckTimer >= 0.25)
		{
			redstoneCheckTimer = glm::mod(redstoneCheckTimer, 0.25);
			if (world.updateRedstoneCables())
			{
				redstoneIdleTimer = 0.0;
				updateRendererData();
			}
			else
			{
				redstoneIdleTimer += 0.25;
				if (redstoneIdleTimer >= 30.0)
				{
					world.stopRedstoneTicks();
					redstoneCheckTimer = 0.0;
					redstoneIdleTimer = 0.0;
				}
			}
		}
	}
	else
	{
		redstoneCheckTimer = 0.0;
		redstoneIdleTimer = 0.0;
	}

	}
	float speed = playerSpeed * (keys.ctrl ? playerSprintMod : 1.0f);
	bool allowMovementInput = !keys.tab && !(renderType == 2 && keys.mmb);
	if (world.getDimensions() < 4) keys.q = keys.e = false;
	if (world.getDimensions() < 5) keys.r = keys.f = false;
	if (renderType == 2) keys.w = keys.s = false;
	m5::vec5 sideMove{0};
	if (renderType == 2 && allowMovementInput)
		for (int pair=0; pair<world.getDimensions()-1; ++pair)
			for (int axis=1; axis<world.getDimensions(); ++axis)
				sideMove[axis] += float(sideBasis[pair][axis]) * (int(keys.sideControls[pair*2+1])-int(keys.sideControls[pair*2]));

	if (collision)
	{
		movePlayerOutOfBlocks();
	}

	if (flight)
	{
		m5::vec5 moveDir = sideMove;

		if (allowMovementInput && keys.w) moveDir += cam.forward;
		if (allowMovementInput && keys.s) moveDir -= cam.forward;
		if (renderType != 2 && allowMovementInput && keys.a) moveDir += cam.left;
		if (renderType != 2 && allowMovementInput && keys.d) moveDir -= cam.left;
		if (renderType != 2 && allowMovementInput && keys.q) moveDir += cam.over;
		if (renderType != 2 && allowMovementInput && keys.e) moveDir -= cam.over;
		if (renderType != 2 && allowMovementInput && keys.r) moveDir += cam.yonder;
		if (renderType != 2 && allowMovementInput && keys.f) moveDir -= cam.yonder;

		moveDir = m5::normalize(moveDir);

		vel = moveDir * speed;

		if (allowMovementInput && keys.space) vel.a += speed;
		if (allowMovementInput && keys.shift) vel.a -= speed;
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
		}

		// a vector pointing outwards from the front of the player
		m5::vec5 pd = renderType == 2 ? cam.forward : m5::normalize(m5::cross(cam.left, m5::vec5::up(), cam.over, cam.yonder));

		m5::vec5 moveDir = sideMove;

		if (allowMovementInput && keys.w) moveDir += pd;
		if (allowMovementInput && keys.s) moveDir -= pd;
		if (renderType != 2 && allowMovementInput && keys.a) moveDir += cam.left;
		if (renderType != 2 && allowMovementInput && keys.d) moveDir -= cam.left;
		if (renderType != 2 && allowMovementInput && keys.q) moveDir += cam.over;
		if (renderType != 2 && allowMovementInput && keys.e) moveDir -= cam.over;
		if (renderType != 2 && allowMovementInput && keys.r) moveDir += cam.yonder;
		if (renderType != 2 && allowMovementInput && keys.f) moveDir -= cam.yonder;

		moveDir = m5::normalize(moveDir);

		m5::vec5 targetVel = moveDir * speed;
		targetVel.a = vel.a;

		m5::vec5 velDiff = targetVel - vel;

		m5::vec5 movement = velDiff * 24.0f;

		vel += movement * dt;

		if (allowMovementInput && onGround && keys.space)
		{
			onGround = false;
			vel.a += 12.0f;
		}
	}

	for (int axis = world.getDimensions(); axis < 5; ++axis) vel[axis] = 0.0f;
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
			bool awaitingTerrain = false;
			for (int j = 0; j <= 4; ++j)
			{
				float height = playerHeight * j / 4.0f;
				m5::vec5 pos = cam.pos;
				pos.a -= height;
				auto hit = world.dda(pos, velNormalized, velLength, false);
				if (hit.unloaded) {awaitingTerrain=true;break;}
				if (hit.blockId && hit.dist < closestHit.dist)
				{
					closestHit = hit;
					closestHitHeight = height;
				}
			}

			if (awaitingTerrain) {vel = m5::vec5{0.0f};break;}
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
		auto groundCheck = world.dda(feetPos, -m5::vec5::up(), 0.1f, false);
		if (!groundCheck.blockId && !groundCheck.unloaded)
		{
			onGround = false;
		}
	}
	else
	{
		cam.pos += vel * dt;
	}

	for (int axis = world.getDimensions(); axis < 5; ++axis) cam.pos[axis] = 0.5f;
	std::string coordinates = std::to_string(world.getDimensions()) + "D" + (renderType == 2 ? " / Render 2D\n" : "\n");
	if (renderType == 2 && dt > 0.0)
	{
		float distance = m5::dot(cam.pos - previousPosition, -cam.left);
		int direction = std::abs(distance) > 0.01 * dt ? (distance > 0 ? 1 : -1) : 0;
		if (direction == 0 || direction != sideWalkDirection) sideWalkTime = 0.0;
		else sideWalkTime = std::fmod(sideWalkTime + dt, 0.32);
		sideWalkDirection = direction;
	}
	auto row = [&](const char* name, const m5::vec5& value) {
		coordinates += name;
		for (int axis = 0; axis < world.getDimensions(); ++axis)
			coordinates += std::format(" {}:{:+.1f}", (char)('A' + axis), value[axis]);
		coordinates += '\n';
	};
	row("Pos:", cam.pos); row("Up:", cam.up); row("Forward:", cam.forward); row("Left:", cam.left);
	if (world.getDimensions() >= 4) row("Over:", cam.over);
	if (world.getDimensions() >= 5) row("Yonder:", cam.yonder);
	if (world.hasTemporal()) coordinates += std::format("Time t1/t2: {}/{}\n", world.temporalPercent(), 100-world.temporalPercent());
	coordsText.setText(coordinates);

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

	if (renderType == 2) updateSideTarget(s);
	else target = world.dda(cam.pos, cam.forward, 100.0, false);

	m5::ivec5 curRendererCenter{
		(int)glm::floor(cam.pos.a / (float)Chunk::HEIGHT),
		(int)glm::floor(cam.pos.b / (float)Chunk::SIZE),
		(int)glm::floor(cam.pos.c / (float)Chunk::SIZE),
		(int)glm::floor(cam.pos.d / (float)Chunk::SIZE),
		(int)glm::floor(cam.pos.e / (float)Chunk::SIZE)
	};
	if (curRendererCenter != rendererCenter)
	{
		updateRendererData();
	}
}

void StateGame::updateSideTarget(StateManager& s)
{
	target = {};
	sidePlacement.reset();
	int width, height;
	s.getSize(&width, &height);
	auto point = SideView::point(cam.pos, lastMousePos, width, height, -cam.left);
	if (m5::length(point - cam.pos) > 12.0f) return;
	m5::ivec5 cell = m5::floor(point);
	const m5::ivec5 extent{32,8,8,8,8};
	m5::ivec5 chunkPos = m5::floor(m5::vec5(cell) / m5::vec5(extent));
	auto chunk = world.findChunk(chunkPos);
	if (!chunk) return;
	target.blockId = chunk->getBlockFull(m5::uvec5(cell - chunkPos * extent));
	target.blockPos = cell;
	target.pos = point;
	// Only voxel faces intersecting the slice can be selected.
	int axis = 0;
	float sign = 1.0f, nearest = INFINITY;
	for (int component = 0; component < world.getDimensions(); ++component)
	{
		float weight = component == 0 ? 1.0f : std::abs(cam.left[component]);
		if (weight < 0.00001f) continue;
		float offset = point[component] - cell[component] - 0.5f;
		float distance = (0.5f - std::abs(offset)) / weight;
		if (distance < nearest) { nearest = distance; axis = component; sign = offset < 0 ? -1.0f : 1.0f; }
	}
	target.normal[axis] = sign;
	target.side = axis * 2 + (sign > 0 ? 1 : 0);
	sidePlacement = target.blockId == Block::AIR ? cell : cell + m5::ivec5(target.normal);
}

void StateGame::render(StateManager& s)
{
	pumpRendererData();
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
	auto dimensionLight = lightDir;
	for (int axis = world.getDimensions(); axis < 5; ++axis) dimensionLight[axis] = 0;
	rendererShader->setUniform("lightDir", m5::normalize(dimensionLight));
	rendererShader->setUniform("worldDimensions", world.getDimensions());
    rendererShader->setUniform("redirectorId",int(ExtendedBlock::REDIRECTOR_0));
	rendererShader->setUniform("sideView", renderType == 2);
	rendererShader->setUniform("sideHalfSize", SideView::halfSize(wW, wH));
	if (renderType == 2)
	{
		const char* frame = !flight && !onGround ? "jump" : sideWalkDirection == 0 ? "idle" : sideWalkTime < 0.16 ? "walk1" :
			sideWalkDirection < 0 ? "walk2-left" : "walk2-right";
		double phase=0;
		for(int hidden=1;hidden<world.getDimensions()-1;++hidden)for(int axis=1;axis<world.getDimensions();++axis)
			phase+=cam.pos[axis]*sideBasis[hidden][axis]*(1.+hidden*.137);
		rendererShader->setUniform("sidePhase",float(phase));
		auto sprite = Texture::get(std::format("player2d.{}", frame));
		rendererShader->setUniform("playerSprite", sprite->getHandle());
		rendererShader->setUniform("playerSpriteSize", glm::vec2{
			playerHeight * sprite->getSize().a / sprite->getSize().b, playerHeight});
	}
	rendererShader->setUniform("time", (float)glfwGetTime());
	rendererShader->setUniform("target.targeting", target.blockId != 0);
	rendererShader->setUniform("target.blockPos", target.blockPos);
	rendererShader->setUniform("target.side", target.side);
	rendererShader->setUniform("structureBlockId", (uint32_t)ExtendedBlock::STRUCTURE);
	rendererShader->setUniform("structureBoundsVisible", structurePreview && structureSelection.visible);
	rendererShader->setUniform("structureMin", m5::vec5(structureBlockPos + structureSelection.offset));
	rendererShader->setUniform("structureMax", m5::vec5(structureBlockPos + structureSelection.offset + structureSelection.size));
	QuadRendererBasic::render();

	glDisable(GL_DEPTH_TEST);
	ui.render();
	if (structureEditorOpen || inventoryOpen)
	{
		qr.setMode(QuadRenderer::MODE_FILL);
		qr.setPos(wW / 2 - 240, wH / 2 - 250, 480, 500);
		qr.setColor({0.025f, 0.04f, 0.035f, 0.96f});
		qr.render();
	}

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
	if (renderType == 2)
	{
		if (world.getDimensions() >= 3 && keys.mmb && !paused && !console.open && !inventoryOpen && !structureEditorOpen)
		{
			sideAngle = std::remainder(sideAngle + float(xpos - lastMousePos.x) * 0.006f, glm::two_pi<float>());
			int hidden = std::min(world.getDimensions()-2, keys.ctrl ? 4 : keys.alt ? 3 : keys.shift ? 2 : 1);
			SideViewND::rotate<5>(sideBasis, hidden, (xpos-lastMousePos.x)*0.006);
			updateCamDirs();
		}
		lastMousePos = {xpos, ypos};
		return;
	}
	if (structureEditorOpen || inventoryOpen) { lastMousePos = {xpos, ypos}; return; }
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
		if (!keys.alt && world.getDimensions() >= 4)
		{
			m5::rotor5 rotH{ m5::wedge(cam.over, cam.left), hAngleDelta }; // CD
			m5::rotor5 rotV{ m5::wedge(pd, cam.over), vAngleDelta }; // BD

			orientation = rotH * rotV * orientation;
		}
		else if (keys.alt && world.getDimensions() >= 5)
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
	if (structureEditorOpen) return;
	if (codepoint == '!' && !console.open && !paused)
	{
		if (inventoryOpen) closeInventory(s); else showInventory(s);
		return;
	}
	if (inventoryOpen) return;
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
	if (structureEditorOpen) return;
	if (inventoryOpen)
	{
		if (yoff != 0) { inventoryPageIndex += yoff < 0 ? 1 : -1; showInventory(s); }
		return;
	}
	if (keys.alt)
	{
		playerSpeed += yoff * 0.2;
		playerSpeed = glm::max(playerSpeed, 0.0f);
	}
	else
	{
		auto nextSelectable = [this](uint16_t blockId)
			{
				do
				{
					if (blockId < Block::COUNT - 1)
					{
						++blockId;
					}
					else if (blockId < ExtendedBlock::FIRST)
					{
						blockId = ExtendedBlock::FIRST;
					}
					else
					{
						blockId = blockId < ExtendedBlock::COUNT - 1 ? blockId + 1 : Block::AIR + 1;
					}
				} while ((ExtendedBlock::isExtended(blockId) && !ExtendedBlock::isSelectable(blockId)) ||
					std::find(inventoryExcluded.begin(), inventoryExcluded.end(), blockId) != inventoryExcluded.end());
				return blockId;
			};
		auto prevSelectable = [this](uint16_t blockId)
			{
				do
				{
					if (blockId > ExtendedBlock::FIRST)
					{
						--blockId;
					}
					else if (blockId == ExtendedBlock::FIRST)
					{
						blockId = Block::COUNT - 1;
					}
					else
					{
						blockId = blockId > Block::AIR + 1 ? blockId - 1 : ExtendedBlock::COUNT - 1;
					}
				} while ((ExtendedBlock::isExtended(blockId) && !ExtendedBlock::isSelectable(blockId)) ||
					std::find(inventoryExcluded.begin(), inventoryExcluded.end(), blockId) != inventoryExcluded.end());
				return blockId;
			};

		if (yoff < 0)
		{
			selectBlock(nextSelectable(selectedBlock));
		}
		else if (yoff > 0)
		{
			selectBlock(prevSelectable(selectedBlock));
		}
	}
}

void StateGame::mouseButtonInput(StateManager& s, int button, int action, int mods)
{
	if (structureEditorOpen || inventoryOpen || paused || console.open) return;
	if (action == GLFW_REPEAT) return;
	if (renderType == 2) updateSideTarget(s);

	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		keys.lmb = (action == GLFW_PRESS);
			if (keys.lmb && target.blockId != Block::AIR)
			{
				if (ExtendedBlock::isCrystalDoor(target.blockId))
				{
				m5::ivec5 pairPos = target.blockPos;
				++pairPos.a;
				if (!ExtendedBlock::isCrystalDoor(world.getBlock(pairPos)))
				{
					pairPos = target.blockPos;
					--pairPos.a;
				}
				if (ExtendedBlock::isCrystalDoor(world.getBlock(pairPos)))
				{
					world.editTemporalBlock(pairPos, Block::AIR);
					}
				}
			world.editTemporalBlock(target.blockPos, Block::AIR);
			if (target.blockId == ExtendedBlock::STRUCTURE)
			{
				auto p = target.blockPos;
				structureSelections.erase({p.a,p.b,p.c,p.d,p.e});
				if (p == structureBlockPos) structurePreview = false;
			}
			updateRendererData();
			}
	} break;
	case GLFW_MOUSE_BUTTON_MIDDLE: keys.mmb = (action == GLFW_PRESS); break;
	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		keys.rmb = (action == GLFW_PRESS);
		if (keys.rmb && (target.blockId != Block::AIR || (renderType == 2 && sidePlacement)))
		{
			if (target.blockId == ExtendedBlock::STRUCTURE)
			{
				openStructureEditor(s, target.blockPos);
				return;
			}
			if (ExtendedBlock::isLever(target.blockId))
			{
				world.editTemporalBlock(target.blockPos, ExtendedBlock::toggledLever(target.blockId));
				updateRendererData();
				return;
			}
			if (ExtendedBlock::isRepeater(target.blockId))
			{
				world.editTemporalBlock(target.blockPos, ExtendedBlock::nextRepeaterMode(target.blockId));
				updateRendererData();
				return;
			}
			if (ExtendedBlock::isRedirector(target.blockId))
			{
				uint16_t nextState = ExtendedBlock::toggledRedirectorSide(target.blockId, target.side);
				world.editTemporalBlock(target.blockPos, nextState);
				updateRendererData();
				return;
			}

			uint16_t blockToPlace = selectedBlock == ExtendedBlock::LEVER_ON ? ExtendedBlock::LEVER_OFF : selectedBlock;
			m5::ivec5 placePos = target.blockPos + target.normal;
			if (renderType == 2 && sidePlacement) placePos = *sidePlacement;
			if (renderType == 2 && world.getBlock(placePos) != Block::AIR) return;
			if (blockToPlace == ExtendedBlock::CRYSTAL_DOOR)
			{
				if (world.getBlock(placePos) != Block::AIR)
				{
					return;
				}

				m5::ivec5 pairPos = placePos;
				++pairPos.a;
				if (world.getBlock(pairPos) != Block::AIR)
				{
					pairPos = placePos;
					--pairPos.a;
				}
				if (world.getBlock(pairPos) != Block::AIR)
				{
					return;
				}

				world.editTemporalBlock(placePos, ExtendedBlock::CRYSTAL_DOOR, true);
				world.editTemporalBlock(pairPos, ExtendedBlock::CRYSTAL_DOOR, true);
			}
			else
			{
				world.editTemporalBlock(placePos, blockToPlace, true);
			}
			updateRendererData();
		}
	} break;
	}
}

void StateGame::keyInput(StateManager& s, int key, int scancode, int action, int mods)
{
	if (inventoryOpen)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) closeInventory(s);
		return;
	}
	if (structureEditorOpen)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) closeStructureEditor(s);
		return;
	}
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

	if (!paused && action == GLFW_PRESS && (key == GLFW_KEY_KP_1 || key == GLFW_KEY_KP_2))
	{
		if (world.changeTemporalSector(key == GLFW_KEY_KP_1 ? 1 : -1))
		{
			movePlayerOutOfBlocks();
			updateRendererData();
		}
		return;
	}
	if (key == GLFW_KEY_TAB)
	{
		keys.tab = (action == GLFW_PRESS);
		if (!keys.tab)
		{
			hyperplaneShortcutFirstComp = -1;
		}
		return;
	}

	auto shortcutAxisComponent = [](int key, int scancode) -> int
	{
		const char* keyName = glfwGetKeyName(key, scancode);
		char label = keyName && keyName[0] && !keyName[1] ? (char)std::tolower((unsigned char)keyName[0]) : '\0';
		switch (label)
		{
		case 'x': return 1; // B / X
		case 'z': return 2; // C / Z
		case 'w': return 3; // D / W
		case 'v': return 4; // E / V
		default: return -1;
		}
	};

	if (keys.tab && action == GLFW_PRESS)
	{
		int comp = shortcutAxisComponent(key, scancode);
		if (comp >= world.getDimensions()) return;
		if (comp != -1 && comp < world.getDimensions())
		{
			if (renderType == 2)
			{
				sideAngle = comp == 1 ? 0.0f : glm::half_pi<float>();
				SideViewND::align<5>(sideBasis,comp,world.getDimensions());
				HyperplaneAlignment::centerHorizontal(cam.pos,world.getDimensions());
				for(int axis=1;axis<5;++axis)vel[axis]=0;
				keys.sideControls={};
				keys.a = keys.d = keys.w = keys.s = false;
				keys.sideHiddenNegative = keys.sideHiddenPositive = false;
				hyperplaneShortcutFirstComp = -1;
				updateCamDirs();
				updateRendererData();
				return;
			}
			if (hyperplaneShortcutFirstComp == -1)
			{
				hyperplaneShortcutFirstComp = comp;
			}
			else if (hyperplaneShortcutFirstComp != comp)
			{
				alignToPlane(hyperplaneShortcutFirstComp, comp);
				hyperplaneShortcutFirstComp = -1;
			}
			return;
		}
	}

	if (renderType == 2)
	{
		const char* name = glfwGetKeyName(key, scancode);
		char label = name && name[0] && !name[1] ? (char)std::tolower((unsigned char)name[0]) : '\0';
		int control = SideViewND::control(label);
		if(control >= 0 && !(key==GLFW_KEY_S && (mods&GLFW_MOD_CONTROL) && (mods&GLFW_MOD_SHIFT))) {
			keys.sideControls[control] = action == GLFW_PRESS;
			return;
		}
	}
	switch (key)
	{
	//case GLFW_KEY_0:
	case GLFW_KEY_1:
	case GLFW_KEY_2:
	case GLFW_KEY_3:
	case GLFW_KEY_4:
	case GLFW_KEY_5:
	case GLFW_KEY_6:
	case GLFW_KEY_7:
	{
		if (action == GLFW_PRESS)
		{
			if (mods & GLFW_MOD_SHIFT) break;
			auto blocks = inventoryBlocks(true);
			size_t index = key - GLFW_KEY_1;
			if (index < blocks.size()) selectBlock(blocks[index]);
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
	case GLFW_KEY_LEFT_SHIFT:
	case GLFW_KEY_RIGHT_SHIFT: keys.shift = (action == GLFW_PRESS); break;
	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_CONTROL: keys.ctrl = (action == GLFW_PRESS); break;
	case GLFW_KEY_LEFT_ALT:
	case GLFW_KEY_RIGHT_ALT: keys.alt = (action == GLFW_PRESS); break;
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
	if (cursor + world.getDimensions() > args.size())
	{
		return std::nullopt;
	}

	m5::vec5 result{0.5f};

	try
	{
		for (int i = 0; i < world.getDimensions(); ++i)
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

	cursor += world.getDimensions();

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
				"- setBlock <pos> <id> - Sets a block at a position.\n"
				"- fill <posA> <posB> <id> - Fills a region with a block.\n"
				"- tp <pos> - Teleports the player to a position.\n"
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
			auto id = intArg(args, cursor, 0, ExtendedBlock::COUNT - 1);
			if (!id.has_value() || id.value() == Block::EXTENDED_MARKER)
			{
				return false;
			}

			world.editTemporalBlock(pos.value(), id.value(), id.value() != Block::AIR);
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
			auto id = intArg(args, cursor, 0, ExtendedBlock::COUNT - 1);
			if (!id.has_value() || id.value() == Block::EXTENDED_MARKER)
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
								world.editTemporalBlock(m5::ivec5{ a,b,c,d,e }, (uint16_t)id.value(), id.value() != Block::AIR);
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
			for (int axis = world.getDimensions(); axis < 5; ++axis) cam.pos[axis] = 0.5f;

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

			alignToPlane(compA, compB);

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
	time_t t = time(0);
	tm tm;
	localtime_s(&tm, &t);
	
	std::stringstream timeStr;
	timeStr << std::put_time(&tm, "%H:%M:%S");

	console.log += '\n';
	if (includeTime)
	{
		console.log += std::format("[{}]: ", timeStr.str());
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

World* StateGame::createWorld(uint8_t edgeLength, uint64_t seed, bool flat, bool caves, int dimensions, int renderType)
{
	this->renderType = worldRenderType(dimensions, renderType);
	keys = {};
	sideAngle = 0.0f;
	SideViewND::align<5>(sideBasis,1,dimensions);
	sideVisible.clear();
	structureEditorOpen = false;
	inventoryOpen = false;
	structurePreview = false;
	structureCapturing = false;
	structurePlacement.reset();
	structureCapturePositions.clear();
	structureCaptureChunks.clear();
	structureSelections.clear();
	renderJobs.clear();
	rendererDirty = true;
	world = World{ edgeLength, seed, flat, caves, dimensions };
	renderPlaneComps = {1, 2};
	structureLibraryStamp.clear();
	structureRefresh = {}; structureReloadTimer = 0;
	rendererCenter = m5::ivec5{ INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN };
	return &world;
}

void StateGame::setWorldPath(std::filesystem::path path)
{
	worldPath = path;
	try
	{
		auto snapshot = worldPath / "structure-generation.json";
		world.loadStructureHistory(std::filesystem::exists(snapshot)
			? Structures::readHistory(snapshot) : Structures::toJson(Structures::Catalog{}));
		reloadStructures();
	}
	catch (const std::exception& error) { print(std::string("Structures: ") + error.what()); }
	loadStructureSelections();
}

void StateGame::updateCamDirs()
{
	if (renderType == 2)
	{
		cam.up = {1,0,0,0,0}; cam.left = cam.forward = m5::vec5{0};
		for(int axis=1;axis<5;++axis) {cam.left[axis]=-float(sideBasis[0][axis]);cam.forward[axis]=float(sideBasis[1][axis]);}
		cam.over = {0,0,0,1,0}; cam.yonder = {0,0,0,0,1};
		return;
	}
	// it's probably faster to convert to a matrix to get the vectors
	cam.up = m5::normalize(orientation.rotate(m5::vec5::up()));
	cam.forward = m5::normalize(orientation.rotate(m5::vec5::forward()));
	cam.left = m5::normalize(orientation.rotate(m5::vec5::left()));
	cam.over = m5::normalize(orientation.rotate(m5::vec5::over()));
	cam.yonder = m5::normalize(orientation.rotate(m5::vec5::yonder()));
	for (int axis = world.getDimensions(); axis < 5; ++axis)
	{
		cam.up[axis] = cam.forward[axis] = cam.left[axis] = 0;
		if (world.getDimensions() >= 4) cam.over[axis] = 0;
	}
	cam.up = m5::normalize(cam.up);
	cam.forward = m5::normalize(cam.forward);
	cam.left = m5::normalize(cam.left);
}

void StateGame::enableCursor(GLFWwindow* window)
{
	glfwSetCursorPos(window, lastMousePos.x, lastMousePos.y);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void StateGame::disableCursor(GLFWwindow* window)
{
	glfwGetCursorPos(window, &lastMousePos.x, &lastMousePos.y);

	//glfwSetCursorPos(window, 0.0, 0.0);
	glfwSetInputMode(window, GLFW_CURSOR, renderType == 2 ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void StateGame::save()
{
	{
		auto infoPath = worldPath / "info.json";
		auto info = std::filesystem::exists(infoPath) ? nlohmann::json::parse(std::ifstream(infoPath)) : nlohmann::json::object();
		info["temporalDimensions"] = world.hasTemporal() ? 2 : 1;
		Structures::writeJson(infoPath, info);
	}
	world.saveTemporal(worldPath / "temporal.bin");
	Structures::writeHistory(worldPath / "structure-generation.json", world.saveStructureHistory());
	saveStructureSelections();
	std::ofstream{ worldPath / "player.json" } << nlohmann::json{
		{ "pos", cam.pos.toJson() },
		{ "orientation", orientation.toJson() },
		{ "sideAngle", sideAngle },
		{ "sideBasis", sideBasis },
		{ "renderPlane", {renderPlaneComps.x, renderPlaneComps.y} },
	};

	const uint32_t magic = 0x50445035; // 5PDP
	const uint32_t version = 3;
	constexpr uint64_t CHUNKS_PER_FILE = 100;
	std::filesystem::path chunksDir = worldPath / "chunks";
	std::filesystem::create_directories(chunksDir);
	for (const auto& entry : std::filesystem::directory_iterator{ chunksDir })
	{
		if (entry.is_regular_file() && entry.path().extension() == ".bin")
		{
			std::filesystem::remove(entry.path());
		}
	}

	auto savedChunks = world.getSavedChunkDiffs();
	uint64_t fileIndex = 0;
	for (uint64_t start = 0; start < savedChunks.size(); start += CHUNKS_PER_FILE, ++fileIndex)
	{
		uint64_t count = glm::min<uint64_t>(CHUNKS_PER_FILE, savedChunks.size() - start);
		std::filesystem::path chunkPath = chunksDir / std::format("chunks_{:06}.bin", fileIndex);
		std::ofstream chunkFile{ chunkPath, std::ios::binary };
		chunkFile.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
		chunkFile.write(reinterpret_cast<const char*>(&version), sizeof(version));
		chunkFile.write(reinterpret_cast<const char*>(&count), sizeof(count));

		for (uint64_t i = 0; i < count; ++i)
		{
			const auto& diff = savedChunks[start + i];
			const m5::ivec5& pos = diff.pos;
			uint32_t changeCount = (uint32_t)diff.changes.size();
			int32_t coords[5]{ pos.a, pos.b, pos.c, pos.d, pos.e };
			chunkFile.write(reinterpret_cast<const char*>(coords), sizeof(coords));
			chunkFile.write(reinterpret_cast<const char*>(&changeCount), sizeof(changeCount));
			chunkFile.write(reinterpret_cast<const char*>(diff.changes.data()), sizeof(World::BlockChange) * diff.changes.size());
		}
	}

	print("Saved the world.");
}

std::vector<uint16_t> StateGame::inventoryBlocks(bool enabledOnly) const
{
	std::vector<uint16_t> result;
	for (uint16_t id = 1; id < ExtendedBlock::COUNT; ++id)
	{
		if (id >= Block::COUNT && (!ExtendedBlock::isExtended(id) || !ExtendedBlock::isSelectable(id))) continue;
		if (enabledOnly && std::find(inventoryExcluded.begin(), inventoryExcluded.end(), id) != inventoryExcluded.end()) continue;
		result.push_back(id);
	}
	return result;
}

void StateGame::closeInventory(StateManager& s)
{
	inventoryOpen = false;
	keys = {};
	s.setUiPage(nullptr);
	disableCursor(s.getWindow());
	saveSettings();
}

void StateGame::showInventory(StateManager& s)
{
	inventoryOpen = true;
	inventoryRefresh = false;
	keys = {};
	vel = {};
	inventoryPage.clear();
	inventoryPage.init(s.getWindow());
	auto blocks = inventoryBlocks();
	int pages = ((int)blocks.size() + 7) / 8;
	inventoryPageIndex = std::clamp(inventoryPageIndex, 0, pages - 1);
	auto place = [&](ui::element& element, int x, int y)
	{
		element.setAlignX(ui::ALIGN_CENTER_X);
		element.setAlignY(ui::ALIGN_CENTER_Y);
		element.setOffsetX(x); element.setOffsetY(y);
		inventoryPage.addElem(&element);
	};
	inventoryTitle.setText("Inventory");
	place(inventoryTitle, 0, -218);
	inventoryCounter.setText(std::to_string(inventoryPageIndex + 1) + " / " + std::to_string(pages));
	inventoryCounter.setSize(1);
	place(inventoryCounter, 0, 165);
	for (int row = 0; row < 8; ++row)
	{
		int index = inventoryPageIndex * 8 + row;
		if (index >= blocks.size()) break;
		auto id = blocks[index];
		auto& check = inventoryChecks[row];
		check.setText(id < Block::COUNT ? Block::names[id] : ExtendedBlock::name(id));
		check.setChecked(std::find(inventoryExcluded.begin(), inventoryExcluded.end(), id) == inventoryExcluded.end());
		check.setAction([this, id, row](bool enabled)
		{
			if (!enabled && inventoryBlocks(true).size() == 1) { inventoryChecks[row].setChecked(true); return; }
			std::erase(inventoryExcluded, id);
			if (!enabled) inventoryExcluded.push_back(id);
			if (!enabled && selectedBlock == id) selectBlock(inventoryBlocks(true).front());
			saveSettings();
		});
		// Left-align variable-length checkbox labels within a fixed row.
		place(check, 0, -164 + row * 40);
		int x, y, w, h;
		check.getBounds(&inventoryPage, &x, &y, &w, &h);
		check.setOffsetX(-120 + w / 2);
		auto& icon = inventoryIcons[row];
		uint16_t atlasId = id;
		const Texture* texture = nullptr;
		if (ExtendedBlock::isExtended(id))
		{
			const auto& data = ExtendedBlock::data(id);
			texture = Texture::get(std::string(data.iconTextureName));
			atlasId = data.iconBlock;
		}
		icon.setTexture(texture ? texture : Texture::get("blockIcons.png"));
		icon.setClip(texture ? 0.0f : (atlasId - 1) / 16.0f, 0, texture ? 1.0f : 1.0f / 16.0f, 1.0f);
		icon.setScale(1.5f, 1.5f);
		place(icon, -166, -164 + row * 40);
	}
	for (int row = 0; row < 8 && inventoryPageIndex * 8 + row < blocks.size(); ++row)
	{
		inventoryPage.removeElem(&inventoryChecks[row]);
		inventoryPage.addElem(&inventoryChecks[row]);
	}
	inventoryPrevious.setText("<"); inventoryPrevious.setSize(40, 32);
	inventoryPrevious.setAction([this] { --inventoryPageIndex; inventoryRefresh = true; });
	place(inventoryPrevious, -75, 165);
	inventoryNext.setText(">"); inventoryNext.setSize(40, 32);
	inventoryNext.setAction([this] { ++inventoryPageIndex; inventoryRefresh = true; });
	place(inventoryNext, 75, 165);
	inventoryClose.setText("Close"); inventoryClose.setSize(120, 32);
	inventoryClose.setAction([this, &s] { closeInventory(s); });
	place(inventoryClose, 0, 212);
	enableCursor(s.getWindow());
	s.setUiPage(&inventoryPage);
}

void StateGame::selectBlock(uint16_t blockId)
{
	selectedBlock = blockId;
	if (ExtendedBlock::isExtended(blockId))
	{
		selectedBlockText.setText("Selected Block: " + std::string{ ExtendedBlock::name(blockId) });
		const auto& blockData = ExtendedBlock::data(blockId);
		if (!blockData.iconTextureName.empty() && Texture::get(std::string{ blockData.iconTextureName }))
		{
			selectedBlockImage.setTexture(Texture::get(std::string{ blockData.iconTextureName }));
			selectedBlockImage.setClip(0, 0, 1.0f, 1.0f);
		}
		else
		{
			selectedBlockImage.setTexture(Texture::get("blockIcons.png"));
			uint8_t iconBlock = blockData.iconBlock;
			selectedBlockImage.setClip((float)(iconBlock - 1) / 16.0f, 0, 1.0f / 16.0f, 1.0f);
		}
	}
	else
	{
		selectedBlockText.setText("Selected Block: " + std::string{ Block::names[blockId] });
		selectedBlockImage.setTexture(Texture::get("blockIcons.png"));
		selectedBlockImage.setClip((float)(blockId - 1) / 16.0f, 0, 1.0f / 16.0f, 1.0f);
	}
}

void StateGame::alignToPlane(int compA, int compB)
{
	if (compA < 1 || compB < 1 || compA == compB || compA >= world.getDimensions() || compB >= world.getDimensions())
	{
		print("Hyperplane unavailable in this world dimension.");
		return;
	}
	if (compA > compB)
	{
		std::swap(compA, compB);
	}

	renderPlaneComps = { compA, compB };
	HyperplaneAlignment::centerHorizontal(cam.pos,world.getDimensions());
	for(int axis=1;axis<5;++axis)vel[axis]=0;
	if (renderType == 2) {sideAngle = 0.0f;SideViewND::align<5>(sideBasis,compA,world.getDimensions());}
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
	updateRendererData();
}

using ChunkRendering::SECTIONS;
using ChunkRendering::VERTICAL_TEXELS;

void StateGame::updateRendererData()
{
	rendererDirty = true;
}

void StateGame::pumpRendererData()
{
	rendererDirty |= world.collectGeneratedChunks(2);
	rendererCenter = {
		(int)glm::floor(cam.pos.a / (float)Chunk::HEIGHT),
		(int)glm::floor(cam.pos.b / (float)Chunk::SIZE),
		(int)glm::floor(cam.pos.c / (float)Chunk::SIZE),
		(int)glm::floor(cam.pos.d / (float)Chunk::SIZE),
		(int)glm::floor(cam.pos.e / (float)Chunk::SIZE)
	};
	m5::ivec5 renderRadiusChunks{ 1, 0, 0, 0, 0 };
	if (cam.pos.a < 0.0f)
	{
		renderRadiusChunks.a = 2;
		renderRadiusChunks[renderPlaneComps.x] = 1;
		renderRadiusChunks[renderPlaneComps.y] = 1;
	}
	else
	{
		renderRadiusChunks[renderPlaneComps.x] = 2;
		renderRadiusChunks[renderPlaneComps.y] = 2;
	}
	if (renderType == 2) for(int axis=1;axis<world.getDimensions();++axis)
		renderRadiusChunks[axis]=std::abs(cam.left[axis])<1e-8f?0:int(std::ceil(16*std::abs(cam.left[axis])/8))+1;
	for (int axis = world.getDimensions(); axis < 5; ++axis) renderRadiusChunks[axis] = 0;
	m5::ivec5 minC = rendererCenter - renderRadiusChunks;
	m5::ivec5 maxC = rendererCenter + renderRadiusChunks;
	if (cam.pos.a < 0.0f)
	{
		maxC.a = rendererCenter.a + 2;
		maxC.a = glm::min(maxC.a, -1);
	}
	else if (cam.pos.a >= 8.0f)
	{
		minC.a = glm::max(minC.a, 0);
		maxC.a = glm::max(maxC.a, world.getNaturalTerrainMaxChunkA());
	}
	else
	{
		minC.a = glm::min(minC.a, 0);
		maxC.a = glm::max(maxC.a, 0);
	}
	if (renderType == 2)
	{
		minC.a = (int)std::floor((cam.pos.a - 11.0f) / Chunk::HEIGHT);
		maxC.a = (int)std::floor((cam.pos.a + 11.0f) / Chunk::HEIGHT);
	}
	m5::ivec5 size = maxC - minC + 1;

	bool tableChanged = rendererDirty || minC != rendererMin || size != rendererSize;
	rendererDirty = false;
	bool handlesChanged = false;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
	int uploads = 0;
	for (auto it = renderJobs.begin(); it != renderJobs.end();)
	{
		if (uploads >= 4 || std::chrono::steady_clock::now() >= deadline) break;
		if (it->result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			++it;
			continue;
		}
		auto packed = it->result.get();
		Chunk* chunk = const_cast<Chunk*>(world.findChunk(it->pos));
		int s = it->section;
		it = renderJobs.erase(it);
		if (!chunk || chunk->shouldUpdateRenderer[s]) continue;

		if (!packed.empty)
		{
			if (chunk->rendererTexIndices[s] && chunk->rendererTexIndices[s] <= blockDataBuffers.size())
			{
				blockDataBuffers[chunk->rendererTexIndices[s] - 1].uploadData(Chunk::SIZE, Chunk::SIZE,
					Chunk::SIZE * Chunk::SIZE * VERTICAL_TEXELS, packed.blocks.data());
				chunk->rendererHandleIndices[s] = chunk->rendererTexIndices[s];
			}
			else
			{
				blockDataBuffers.emplace_back(TextureBuffer{ Chunk::SIZE, Chunk::SIZE,
					Chunk::SIZE * Chunk::SIZE * VERTICAL_TEXELS, TextureFormat::RGBA32u, packed.blocks.data() });
				blockDataHandles.emplace_back(blockDataBuffers.back().getHandle());
				chunk->rendererTexIndices[s] = blockDataBuffers.size();
				chunk->rendererHandleIndices[s] = blockDataHandles.size();
				handlesChanged = true;
			}
		}
		else
		{
			// Keep the allocation for a later edit, but hide the empty section.
			chunk->rendererHandleIndices[s] = 0;
		}
		tableChanged = true;
		++uploads;
	}

	std::vector<m5::ivec5> visible;
	for (int ca = 0; ca < size.a; ++ca)
	for (int b = 0; b < size.b; ++b)
	for (int c = 0; c < size.c; ++c)
	for (int d = 0; d < size.d; ++d)
	for (int e = 0; e < size.e; ++e)
	{
		visible.push_back(minC + m5::ivec5{ ca,b,c,d,e });
	}

	// Nearby chunks first, with a bounded number of snapshots and queued tasks.
	if(renderType == 2) {
		SideViewND::Vector<5> eye{},right{};
		for(int axis=1;axis<5;++axis){eye[axis]=cam.pos[axis]/8.;right[axis]=-cam.left[axis]/8.;}
		visible.clear();
		for(auto column:SideViewND::columns(eye,right,16.))for(int a=minC.a;a<=maxC.a;++a)
			visible.push_back({a,column[1],column[2],column[3],column[4]});
		if(visible!=sideVisible){tableChanged=true;sideVisible=visible;}
	}
	std::stable_sort(visible.begin(), visible.end(), [this](const auto& left, const auto& right)
	{
		auto distance = [this](const auto& pos)
		{
			int result = 0;
			for (int axis = 0; axis < 5; ++axis) result += std::abs(pos[axis] - rendererCenter[axis]);
			return result;
		};
		return distance(left) < distance(right);
	});
	int submitted = 0;
	for (auto pos : visible)
	{
		world.requestChunk(pos);
		auto chunk = world.findChunk(pos);
		if (!chunk || renderJobs.size() >= 12 || submitted >= 4) continue;
		for (int s = 0; s < SECTIONS; ++s)
		{
			if (!chunk->shouldUpdateRenderer[s]) continue;
			bool pending = std::any_of(renderJobs.begin(), renderJobs.end(), [pos, s](const RenderJob& job)
				{ return job.pos == pos && job.section == s; });
			if (pending) continue;
			auto snapshot = std::make_shared<Chunk>();
			snapshot->blocks = chunk->blocks;
			snapshot->extendedBlocks = chunk->extendedBlocks;
			const_cast<Chunk*>(chunk)->shouldUpdateRenderer[s] = false;
			renderJobs.push_back({ pos, s, BackgroundWork::rendering().addTaskFuture(
				[snapshot, s]() { return ChunkRendering::pack(*snapshot, s); }) });
			++submitted;
		}
	}

	if (handlesChanged)
	{
		// Grow geometrically instead of reallocating the SSBO for every new chunk.
		size_t required = sizeof(uint64_t) * blockDataHandles.size();
		if (required > blockDataHandlesBuffer.getSize())
		{
			blockDataHandlesBuffer.fit(std::max(required, std::max(size_t{ 1024 }, blockDataHandlesBuffer.getSize() * 2)));
		}
		blockDataHandlesBuffer.uploadData(0, sizeof(uint64_t) * blockDataHandles.size(), blockDataHandles.data());
	}
	if (tableChanged)
	{
		uint32_t sectionSize = size.a * size.b * size.c * size.d * size.e;
		std::vector<uint32_t> chunks(sectionSize * SECTIONS, 0);
		for (auto pos : visible)
		{
			auto chunk = world.findChunk(pos);
			if (!chunk) continue;
			m5::uvec5 uPos = pos - minC;
			uint32_t ind = uPos.b + uPos.c * size.b + uPos.d * size.b * size.c
				+ uPos.e * size.b * size.c * size.d + uPos.a * size.b * size.c * size.d * size.e;
			for (int s = 0; s < SECTIONS; ++s) chunks[ind * SECTIONS + s] = (uint32_t)chunk->rendererHandleIndices[s];
		}
		chunksBuffer.uploadData(size.b, size.c, size.d * size.e * size.a, chunks.data());
		rendererShader->setUniform("chunksMinBound", minC);
		rendererShader->setUniform("chunksSize", size);
		rendererShader->setUniform("chunks", chunksBuffer.getHandle());
		rendererMin = minC;
		rendererSize = size;
	}
}

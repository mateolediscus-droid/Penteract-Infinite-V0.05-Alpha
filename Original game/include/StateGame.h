#pragma once
#include "StateManager.h"
#include "GPUBuffer.h"
#include "Math5D.h"
#include "World.h"
#include "TextureBuffer.h"
#include <optional>

class Shader;

class StateGame : public State
{
public:

	static StateGame* instance();

	void init(StateManager& s) override;
	void close(StateManager& s) override;
	void pause(StateManager& s) override;
	void resume(StateManager& s) override;
	void update(StateManager& s, double dt) override;
	void render(StateManager& s) override;
	void mouseInput(StateManager& s, double xpos, double ypos) override;
	void scrollInput(StateManager& s, double xoff, double yoff) override;
	void mouseButtonInput(StateManager& s, int button, int action, int mods) override;
	void keyInput(StateManager&, int key, int scancode, int action, int mods) override;
	void windowResize(StateManager&, int width, int height) override;
	void charInput(StateManager& s, uint32_t codepoint) override;

	// creates a world with a given edge length,
	// returning a pointer so the caller can initialize the world data.
	World* createWorld(uint8_t edgeLength);
	void setWorldPath(std::filesystem::path path);

private:

	static StateGame instanceObj;
	~StateGame() {}

	struct alignas(16) Camera
	{
		m5::vec5 pos; uint32_t _pad0[3]{ 0 };
		m5::vec5 up; uint32_t _pad2[3]{ 0 };
		m5::vec5 forward; uint32_t _pad3[3]{ 0 };
		m5::vec5 left; uint32_t _pad1[3]{ 0 };
		m5::vec5 over; uint32_t _pad4[3]{ 0 };
		m5::vec5 yonder; uint32_t _pad5[3]{ 0 };
		float vFov = glm::pi<float>() * 0.5f;
		uint32_t _pad6[3]{ 0 };
	};

	const float gravity = -32.0f;
	const float playerHeight = 1.8f;
	bool collision = true;
	bool flight = false;
	m5::vec5 vel{ 0 };
	bool onGround = false;
	float playerSpeed = 7.0f;
	float playerSprintMod = 2.0f;

	World::Collision target;
	glm::mat4 projection3D;
	m5::rotor5 orientation;
	Camera cam;
	GPUBuffer cameraBuf;
	Shader* rendererShader;
	TextureBuffer chunksBuffer;
	std::vector<TextureBuffer> blockDataBuffers;
	std::vector<uint64_t> blockDataHandles;
	GPUBuffer blockDataHandlesBuffer;
	GPUBuffer blockTexturesBuffer;
	GPUBuffer tileTextureHandles;

	void updateCamDirs();

	struct
	{
		bool w;
		bool s;
		bool a;
		bool d;
		bool q;
		bool e;
		bool r;
		bool f;
		bool space;
		bool shift;
		bool ctrl;
		bool alt;
		bool lmb;
		bool mmb;
		bool rmb;
	} keys;

	glm::dvec2 lastMousePos{ 0 };
	void enableCursor(GLFWwindow* window);
	void disableCursor(GLFWwindow* window);

	World world;
	std::filesystem::path worldPath;

	void save();

	m5::vec5 lightDir = m5::normalize({ 7.0f, 2.0f, 1.5f, 1.0f, 1.0f });
	
	// TODO: possibly move this to it's own state, since it adds unnecessary complications to the game state
	ui::page pauseMenu;
	bool paused = false;
	
	ui::text pausedText;
	ui::button backToGameButton;
	ui::button quitToTitleButton;
	ui::button controlsButton;
	ui::checkbox shadowsCheckbox;
	ui::checkbox ambientOcclusionCheckbox;
	ui::checkbox waterCheckbox;
	ui::checkbox scaleUIx2Checkbox;

	ui::page ui;
	ui::text fpsText;
	ui::text coordsText;

	ui::text selectedBlockText;
	ui::image selectedBlockImage;
	uint8_t selectedBlock = Block::GRASS;

	void selectBlock(uint8_t blockId);

	QuadRenderer qr;

	struct
	{
		const int height = 300;
		const int maxLines = height / 8 - 1;
		QuadRenderer qr;
		bool open = true;
		bool blockCharInput = true;

		ui::page ui;

		ui::text logText;
		ui::text inputText;

		std::string log = "";
		std::string input = "";

		int cursorPos = 0;

		int historyIndex = 0;
		int historyCount = 0;
		std::array<std::string, 64> history{ "" };
	} console;
	void updateConsoleInput();
	std::optional<std::string> strArg(const std::vector<std::string>& args, int& cursor);
	std::optional<int64_t> intArg(const std::vector<std::string>& args, int& cursor, int64_t min = INT_MIN, int64_t max = INT_MAX);
	std::optional<double> floatArg(const std::vector<std::string>& args, int& cursor, double min = -INFINITY, double max = INFINITY);
	std::optional<m5::vec5> posArg(const std::vector<std::string>& args, int& cursor);
	void print(std::string_view message, bool includeTime = true);
	void exec(std::string_view cmd);

	void updateRendererData();

	void saveSettings() const;
	void loadSettings();
};
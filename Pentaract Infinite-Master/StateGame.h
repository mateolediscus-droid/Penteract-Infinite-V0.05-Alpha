#pragma once
#include "StateManager.h"
#include "GPUBuffer.h"
#include "Math5D.h"
#include "World.h"
#include "TextureBuffer.h"
#include "ChunkRenderData.h"
#include "SideViewND.h"
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
	World* createWorld(uint8_t edgeLength, uint64_t seed = 0, bool flat = false, bool caves = false, int dimensions = 5, int renderType = 3);
	int getRenderType() const { return renderType; }
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
	int renderType = 3;
	float sideAngle = 0.0f;
	SideViewND::Basis<5> sideBasis{};
	std::vector<m5::ivec5> sideVisible;
	void updateSideTarget(StateManager& s);
	std::optional<m5::ivec5> sidePlacement;
	double sideWalkTime = 0.0;
	int sideWalkDirection = 0;
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
	m5::ivec5 rendererCenter{ INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN };
	using PackedChunk = ChunkRendering::PackedChunk;
	struct RenderJob
	{
		m5::ivec5 pos;
		int section;
		std::future<PackedChunk> result;
	};
	std::vector<RenderJob> renderJobs;
	bool rendererDirty = true;
	m5::ivec5 rendererMin{ INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN };
	m5::ivec5 rendererSize{ 0 };
	void pumpRendererData();

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
		bool tab;
		bool sideHiddenNegative;
		bool sideHiddenPositive;
		std::array<bool,12> sideControls{};
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
	uint16_t selectedBlock = Block::GRASS;
	bool inventoryOpen = false;
	bool inventoryRefresh = false;
	int inventoryPageIndex = 0;
	std::vector<uint16_t> inventoryExcluded;
	ui::page inventoryPage;
	ui::text inventoryTitle, inventoryCounter;
	std::array<ui::checkbox, 8> inventoryChecks;
	std::array<ui::image, 8> inventoryIcons;
	ui::button inventoryPrevious, inventoryNext, inventoryClose;
	std::vector<uint16_t> inventoryBlocks(bool enabledOnly = false) const;
	void showInventory(StateManager& s);
	void closeInventory(StateManager& s);
	int hyperplaneShortcutFirstComp = -1;
	glm::ivec2 renderPlaneComps{ 1, 2 };
	double sandFallTimer = 0.0;
	double redstoneCheckTimer = 0.0;
	double redstoneIdleTimer = 0.0;

	void selectBlock(uint16_t blockId);
	void alignToPlane(int compA, int compB);
	bool playerIntersectsBlock(const m5::vec5& pos, bool* pending = nullptr);
	void movePlayerOutOfBlocks();

	QuadRenderer qr;

	struct
	{
		inline static constexpr int height = 300;
		inline static constexpr int maxLines = height / 8 - 1;
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

	struct StructureSelection
	{
		std::string name = "my_structure";
		m5::ivec5 offset{1, 0, 0, 0, 0};
		m5::ivec5 size{8, 5, 5, 1, 1};
		bool visible = true;
	};
	std::unordered_map<World::ChunkKey, StructureSelection> structureSelections;
	m5::ivec5 structureBlockPos{0};
	StructureSelection structureSelection;
	bool structureEditorOpen = false;
	bool structurePreview = false;
	ui::page structurePage;
	ui::text structureTitle, structureNameLabel, structureOffsetLabel, structureSizeLabel, structureStatus;
	ui::text_input structureNameInput;
	std::array<ui::text, 5> structureAxisLabels;
	std::array<ui::text_input, 5> structureOffsetInputs, structureSizeInputs;
	ui::checkbox structureBoundsCheckbox;
	ui::button structureSaveButton, structureLoadButton, structureCloseButton;
	ui::button structurePreviousButton, structureNextButton;
	void chooseStructureFile(int direction);
	std::optional<Structures::Template> structurePlacement;
	size_t structurePlacementIndex = 0;
	void startStructurePlacement();
	bool structureCapturing = false;
	std::vector<m5::ivec5> structureCapturePositions;
	std::vector<Structures::CapturedChunk> structureCaptureChunks;
	size_t structureCaptureIndex = 0;
	m5::ivec5 structureCaptureMin{0};
	StructureSelection structureCaptureSelection;
	std::future<size_t> structureSaveResult;
	void openStructureEditor(StateManager& s, m5::ivec5 pos);
	void closeStructureEditor(StateManager& s);
	bool readStructureInputs(bool reportError);
	void startStructureCapture();
	void updateStructureEditor();
	void saveStructureSelections();
	void loadStructureSelections();
	void reloadStructures();
	double structureReloadTimer = 0;
	std::string structureLibraryStamp;
	struct StructureRefresh {
		std::string stamp;
		std::shared_ptr<const Structures::Catalog> catalog;
		std::vector<std::string> warnings;
	};
	std::future<StructureRefresh> structureRefresh;
	void pollStructureLibrary(double dt);
};

#pragma once
#include "State.h"
#include "ui.h"
#include "MashPool.h"
#include "OpenSimplexNoise.hpp"
#include <atomic>

struct Chunk;
class World;
class StateGeneratingWorld : public State
{
public:

	// static methods
	// --------------

	static StateGeneratingWorld* instance();

	// State method overrides
	// ----------------------

	bool shouldRenderPrevState() { return true; }

	void init(StateManager& s) override;
	//void close(StateManager& s) override;
	void update(StateManager& s, double dt) override;
	void render(StateManager& s) override;
	void mouseInput(StateManager& s, double xpos, double ypos) override;
	void mouseButtonInput(StateManager& s, int button, int action, int mods) override;

	// class-specific methods
	// ----------------------

	void setDetails(std::string_view worldName, std::string_view worldSeed, uint8_t size, bool flat, bool caves);

private:

	static StateGeneratingWorld instanceObj;
	~StateGeneratingWorld() {}

	std::unique_ptr<OpenSimplexNoise> noise;

	ui::page page;

	ui::text generatingText;
	ui::button cancelButton;

	// for loading bar
	QuadRenderer qr;

	std::string worldName;
	int64_t seed;
	// this is an int for convenient multiplication
	int size = 3;
	bool flat = false;
	bool caves = false;
	World* world;

	MashPool pool{ glm::max<unsigned int>(1, std::thread::hardware_concurrency() - 1) };
	std::atomic_int taskCount = 0;
	int taskCountTotal = 0;

	void generateChunk(Chunk* chunk, glm::u8vec4 chunkPos);
	void generateChunkFlat(Chunk* chunk, glm::u8vec4 chunkPos);
};
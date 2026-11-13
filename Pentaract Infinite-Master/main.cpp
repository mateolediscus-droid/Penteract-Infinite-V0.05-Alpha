#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <ndtf/ndtf.h>
#include "Math5D.h"

#include <string>

#include "StateManager.h"

#include "StateTitleScreen.h"
#include "WorldNDRegistry.h"
#include "NDSourceRoot.h"
#include "json.hpp"
#include <fstream>

#include "QuadRendererBasic.h"
#include "Texture.h"
#include "Shader.h"
#include "ui.h"

#include "audio.h"

#include <Windows.h>

#include "Directory.h"

#ifdef _WIN32
extern "C"
{
	_declspec(dllexport) int NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void handleRawMouseInput(GLFWwindow* window, double xpos, double ypos)
{
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->mouseInput(xpos, ypos);
}
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->scrollInput(xoffset, yoffset);
}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->mouseButtonInput(button, action, mods);
}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->keyInput(key, scancode, action, mods);
}
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->windowResize(width, height);
}
void charCallback(GLFWwindow* window, unsigned int codepoint)
{
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->charInput(codepoint);
}
void fileDropCallback(GLFWwindow* window, int path_count, const char* paths[])
{
	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->fileDrop(path_count, paths);
}

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
	GLsizei length, const char* message, const void* userParam)
{
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	printf("[GL Debug]: ");

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:				printf("[Error]: "); break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	printf("[Deprecated Behaviour]: "); break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	printf("[Undefined Behaviour]: "); break;
	case GL_DEBUG_TYPE_PORTABILITY:			printf("[Portability]: "); break;
	case GL_DEBUG_TYPE_PERFORMANCE:			printf("[Performance]: "); break;
	case GL_DEBUG_TYPE_MARKER:				printf("[Marker]: "); break;
	case GL_DEBUG_TYPE_PUSH_GROUP:			printf("[Push Group]: "); break;
	case GL_DEBUG_TYPE_POP_GROUP:			printf("[Pop Group]: "); break;
	case GL_DEBUG_TYPE_OTHER:				printf("[Other]: "); break;
	}

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:				printf("[API]: "); break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		printf("[Window System]: "); break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	printf("[Shader Compiler]: "); break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:		printf("[Third Party]: "); break;
	case GL_DEBUG_SOURCE_APPLICATION:		printf("[Application]: "); break;
	case GL_DEBUG_SOURCE_OTHER:				printf("[Other]: "); break;
	}
	printf("\n(%i): %s\n", id, message);
}
#endif

int main(int argc,char** argv)
{
	// Compiler/cache validation does not create a window or touch a world.
	if(argc==3 && std::string_view(argv[1])=="--nd-probe"){
		try {int n=std::stoi(argv[2]);return n>=6 && worldNDCompiled(n) && worldNDEntry(n).configure ? 0 : 1;}
		catch(...){return 1;}
	}
	wchar_t executablePath[32768]{};
	if (GetModuleFileNameW(nullptr, executablePath, 32768)) {
		auto dataRoot=std::filesystem::path(executablePath).parent_path();
		auto sourceRoot=dataRoot.parent_path().parent_path();
		// A source checkout keeps authored assets above x64/Release, not in build output.
		if(!std::filesystem::is_directory(dataRoot/"assets") &&
			std::filesystem::is_regular_file(sourceRoot/"5D-Miner.vcxproj") &&
			std::filesystem::is_directory(sourceRoot/"assets"))dataRoot=sourceRoot;
		std::filesystem::current_path(dataRoot);
	}
	wchar_t runtimeRoot[32768]{},runtimeWorld[32768]{};
	GetEnvironmentVariableW(L"PENTERACT_ND_ROOT",runtimeRoot,32768);
	GetEnvironmentVariableW(L"PENTERACT_ND_WORLD",runtimeWorld,32768);
	SetEnvironmentVariableW(L"PENTERACT_ND_ROOT",nullptr);
	SetEnvironmentVariableW(L"PENTERACT_ND_WORLD",nullptr);
	if(runtimeRoot[0])std::filesystem::current_path(runtimeRoot);
	if(argc==2 && std::string_view(argv[1])=="--release-probe") {
		try {
			auto root=std::filesystem::current_path(),source=ndSourceRoot(root);
			for(const auto* p:{"assets/shaders/rendererND.glsl","assets/shaders/sliceMeshND.glsl","assets/textures/icon.ico"})
				if(!std::filesystem::is_regular_file(root/p))return 2;
			for(const auto* p:{"packages","libs","experiments"})if(!std::filesystem::is_directory(source/p))return 3;
			return std::filesystem::is_directory(root/"TextureProfiles")?0:4;
		} catch(...) {return 1;}
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (!glfwInit())
	{
		printf("Failed to initialize GLFW!\n");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Pentaract Infinite - Alpha V0.05", nullptr, nullptr);
	if (!window)
	{
		printf("Failed to initialize the window!\n");
		return 2;
	}

	StateManager stateManager{ window };
	glfwSetWindowUserPointer(window, &stateManager);

	glfwSetInputMode(window, GLFW_STICKY_KEYS, true);

	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, true);
	}
	glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, true);

	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetCursorPosCallback(window, handleRawMouseInput);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetDropCallback(window, fileDropCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetCharCallback(window, charCallback);

	glfwSwapInterval(1);

	glfwMakeContextCurrent(window);
	glewExperimental = true;
	if (glewInit())
	{
		printf("Failed to initialize GLEW!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 3;
	}

	if (!GLEW_ARB_bindless_texture)
	{
		printf("The ARB_bindless_texture OpenGL extension is not supported!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 4;
	}

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QuadRendererBasic::init();

	Shader::load("quad", {
		{ GL_VERTEX_SHADER, "assets/shaders/quad.vert" },
		{ GL_FRAGMENT_SHADER, "assets/shaders/quad.frag" }
	});

	Shader::load("text", {
		{ GL_VERTEX_SHADER, "assets/shaders/text.vert" },
		{ GL_GEOMETRY_SHADER, "assets/shaders/text.geom" },
		{ GL_FRAGMENT_SHADER, "assets/shaders/text.frag" }
	});

	Shader::load("tex", {
		{ GL_VERTEX_SHADER, "assets/shaders/quadBasic.vert" },
		{ GL_FRAGMENT_SHADER, "assets/shaders/tex.frag" }
	});

	Shader::load("renderer", {
		{ GL_VERTEX_SHADER, "assets/shaders/renderer.vert" },
		{ GL_FRAGMENT_SHADER, "assets/shaders/renderer.frag" }
	});

	ui::element::renderInit();

	audio::init();

	if (!Directory::exists("worlds"))
	{
		Directory::create("worlds");
	}

	stateManager.changeState(StateTitleScreen::instance());
	if(runtimeWorld[0]){
		try {
			nlohmann::json info;std::ifstream(std::filesystem::path(runtimeWorld)/"info.json")>>info;
			auto entry=worldNDEntry(info.at("dimensions").get<int>());
			entry.configure(runtimeWorld,info.value<uint64_t>("seed",0),info.value("flat",false),info.value("caves",false));
			stateManager.changeState(entry.instance());
		}catch(const std::exception& error){
			MessageBoxA(nullptr,error.what(),"ND world loading failed",MB_OK|MB_ICONERROR);
			return 5; // Do not render a partially initialized state after a GPU/compiler limit.
		}
	}

	int wW, wH;
	stateManager.getSize(&wW, &wH);
	stateManager.windowResize(wW, wH);

	double frameTime = 0.0;
	double lastTime = glfwGetTime();
	while (stateManager.isRunning())
	{
		double curTime = glfwGetTime();
		double dt = curTime - lastTime;
		lastTime = curTime;

		frameTime += dt;

		if (frameTime > 0.1)
		{
			frameTime = 0.01;
		}

		while (frameTime >= 0.01)
		{
			glfwPollEvents();
			stateManager.update(0.01);
			frameTime -= 0.01;
		}

		stateManager.render();

		while (stateManager.shouldClose() && stateManager.isRunning())
		{
			stateManager.popState();
		}
	}

	audio::deinit();

	Texture::destroy();
	Shader::destroy();
	QuadRendererBasic::destroy();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

#ifndef _DEBUG
int CALLBACK WinMain(
	__in  HINSTANCE hInstance,
	__in  HINSTANCE hPrevInstance,
	__in  LPSTR lpCmdLine,
	__in  int nCmdShow
)
{
	return main(__argc,__argv);
}
#endif

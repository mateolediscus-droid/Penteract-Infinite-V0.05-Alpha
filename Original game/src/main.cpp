#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ndtf/ndtf.h>
#include "Math5D.h"

#include <string>

#include "StateManager.h"

#include "StateTitleScreen.h"

#include "QuadRendererBasic.h"
#include "Texture.h"
#include "Shader.h"
#include "ui.h"

#include "audio.h"

#include "Directory.h"

#include "stb_image.h"

#ifdef _WIN32
extern "C"
{
	_declspec(dllexport) int NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void handleRawMouseInput(GLFWwindow* window, double xpos, double ypos)
{
	int wW, wH;
	glfwGetWindowSize(window, &wW, &wH);
	int fW, fH;
	glfwGetFramebufferSize(window, &fW, &fH);

	StateManager* s = reinterpret_cast<StateManager*>(glfwGetWindowUserPointer(window));
	s->mouseInput(xpos * fW / wW, ypos * fH / wH);
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

int main()
{
#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif

	if (!glfwInit())
	{
		printf("Failed to initialize GLFW!\n");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	const char* sessionType = std::getenv("XDG_SESSION_TYPE");
	if (sessionType && std::string(sessionType) == "wayland")
		glfwWindowHintString(GLFW_WAYLAND_APP_ID, "dev.tr1ngle.pp");

#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Penteract Placer by Mashpoe and Tr1Ngle", nullptr, nullptr);
	if (!window)
	{
		printf("Failed to initialize the window!\n");
		return 2;
	}

	// load the window icon
	{
		GLFWimage icons[4];
		for (int i = 0; i < 4; ++i)
		{
			int size = 16 * (i + 1);
			int c;
			icons[i].pixels = stbi_load(std::format("assets/textures/icon{}.png", size).c_str(), &icons[i].width, &icons[i].height, &c, 4);
		}
		glfwSetWindowIcon(window, 4, icons);
		for (int i = 0; i < 4; ++i)
		{
			stbi_image_free(icons[i].pixels);
		}
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
	if (!gladLoadGL(glfwGetProcAddress))
	{
		printf("Failed to initialize OpenGL!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 3;
	}

	if (!GLAD_GL_ARB_bindless_texture)
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

#ifdef _WIN32
#ifndef _DEBUG
int CALLBACK WinMain(
	__in  HINSTANCE hInstance,
	__in  HINSTANCE hPrevInstance,
	__in  LPSTR lpCmdLine,
	__in  int nCmdShow
)
{
	return main();
}
#endif
#endif

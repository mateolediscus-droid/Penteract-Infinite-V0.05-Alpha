#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>

class QuadRendererBasic
{
private:
	inline static constexpr uint8_t vertices[]
	{
		0,0,
		1,0,
		1,1,
		0,1,
	};
	static uint32_t VAO, VBO;
public:
	static void init();
	static void render(GLenum mode = GL_TRIANGLE_FAN);
	static void destroy();
};
#pragma once

#include <glad/gl.h>
#include "Texture.h"
#include "Shader.h"

#include <glm/mat4x4.hpp>

class QuadRenderer
{
public:

	// rendering mode
	enum Mode
	{
		MODE_FILL = GL_TRIANGLE_FAN,
		MODE_LINES = GL_LINE_LOOP,
	};

	QuadRenderer();
	QuadRenderer(const Shader* s);
	~QuadRenderer();

	void setShader(const Shader* s);

	bool init();

	void cleanup();

	void render();

	// sets the color of the quad
	void setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0);
	void setColor(const glm::vec4& color);
	void setColor(const glm::vec3& color);

	// expects an array of floats to represent 4 rgba colors, a total of 16 elements.
	// one color for each corner
	void setColors(GLfloat* colors);

	void setColors(const std::array<glm::vec4, 4>& colors);

	void setPos(GLint x, GLint y, GLint w, GLint h);
	void setPos(GLint x, GLint y, GLint w, GLint h, GLfloat radians);

	void setMode(Mode renderingMode);

private:

	const Shader* shader;
	GLuint VAO;


	enum bufferIndex {
		VBO_VERTEX,
		VBO_COLOR,
		VBO_MODEL,
		VBO_COUNT,
	};

	GLuint buffers[VBO_COUNT];

	GLenum mode = MODE_FILL;

};
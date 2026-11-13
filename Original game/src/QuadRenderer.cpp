#include "QuadRenderer.h"
#include <glm/gtc/type_ptr.hpp>

QuadRenderer::QuadRenderer(): shader(nullptr), VAO(0), buffers()
{
}

QuadRenderer::QuadRenderer(const Shader* s) : shader(s), VAO(0), buffers()
{
}

QuadRenderer::~QuadRenderer()
{
	cleanup();
}

void QuadRenderer::setShader(const Shader* s)
{
	this->shader = s;
}

bool QuadRenderer::init()
{
	if (VAO != 0)
	{
		cleanup();
	}

	if (shader == nullptr)
		return false;

	constexpr GLfloat vertices[] = {
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
	};

	constexpr GLfloat colors[] = {
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
	};

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(VBO_COUNT, this->buffers);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(this->VAO);

	// position attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	// color attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);

	// matrices take up four attribute locations
	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_MODEL]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4, glm::value_ptr(model), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(0));
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(GLfloat) * 4));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(GLfloat) * 8));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(GLfloat) * 12));
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return true;
}

void QuadRenderer::cleanup()
{
	if (this->VAO == 0)
	{
		return;
	}
	glBindVertexArray(this->VAO);
	glDeleteBuffers(VBO_COUNT, this->buffers);
	glDeleteVertexArrays(1, &this->VAO);
}

void QuadRenderer::render()
{
	shader->use();
	glBindVertexArray(this->VAO);
	glDrawArrays(this->mode, 0, 4);
	glBindVertexArray(0);
}

void QuadRenderer::setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	GLfloat colors[] = {
		r, g, b, a,
		r, g, b, a,
		r, g, b, a,
		r, g, b, a
	};

	glBindVertexArray(this->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadRenderer::setColor(const glm::vec4& color)
{
	setColor(color.r, color.g, color.b, color.a);
}

void QuadRenderer::setColor(const glm::vec3& color)
{
	setColor(color.r, color.g, color.b);
}

void QuadRenderer::setColors(GLfloat* colors)
{
	glBindVertexArray(this->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, colors, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadRenderer::setColors(const std::array<glm::vec4, 4>& colors)
{
	setColors((float*)colors.data());
}

void QuadRenderer::setPos(GLint x, GLint y, GLint w, GLint h)
{
	glBindVertexArray(this->VAO);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(x, y, 0.0f));

	model = glm::scale(model, glm::vec3(w, h, 1.0f));

	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_MODEL]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4, glm::value_ptr(model), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadRenderer::setPos(GLint x, GLint y, GLint w, GLint h, GLfloat radians)
{
	glBindVertexArray(this->VAO);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(x, y, 0.0f));

	model = glm::translate(model, glm::vec3(0.5f * w, 0.5f * h, 0.0f));
	model = glm::rotate(model, radians, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::translate(model, glm::vec3(-0.5f * w, -0.5f * h, 0.0f));

	model = glm::scale(model, glm::vec3(w, h, 1.0f));

	glBindBuffer(GL_ARRAY_BUFFER, this->buffers[VBO_MODEL]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 4, glm::value_ptr(model), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadRenderer::setMode(Mode renderingMode)
{
	this->mode = renderingMode;
}
#include "TextRenderer.h"
#include "Texture.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include "ui.h"

TextRenderer::TextRenderer(const Texture* texture, const Shader* shader) : texture(texture), shader(shader)
{
	init();
}

TextRenderer::TextRenderer() : VAO(0), charVBO(0)
{
}

TextRenderer::~TextRenderer()
{
}

TextRenderer::TextRenderer(TextRenderer&& other) noexcept
{
	move(other);
}

TextRenderer& TextRenderer::operator=(TextRenderer&& other) noexcept
{
	cleanup();

	move(other);

	return *this;
}

void TextRenderer::setTexture(const Texture* t)
{
	texture = t;
}

void TextRenderer::setShader(const Shader* s)
{
	shader = s;
}

void TextRenderer::setCharSize(int w, int h)
{
	charSize = { w, h };
	updateModel();
}

void TextRenderer::setCharSize(const glm::ivec2 size)
{
	charSize = size;
	updateModel();
}

int TextRenderer::getCharWidth()
{
	return charSize.x;
}

int TextRenderer::getCharHeight()
{
	return charSize.y;
}

void TextRenderer::setFontSize(int size)
{
	fontSize = size;
}

int TextRenderer::getFontSize()
{
	return fontSize;
}

void TextRenderer::setColor(float r, float g, float b, float a)
{
	color = { r, g, b, a };
}

void TextRenderer::setColor(const glm::vec4& color)
{
	this->color = color;
}

void TextRenderer::setColor(const glm::vec3& color)
{
	this->color = { color.r, color.g, color.b, 1.0f };
}

void TextRenderer::setText(std::string_view text)
{
	this->text = text;

	glBindVertexArray(this->VAO);

	updateCharVBO();

	updateModel();
}

void TextRenderer::setPos(int x, int y)
{
	pos = { x, y };
	updateModel();
}

void TextRenderer::setPos(const glm::ivec2& pos)
{
	this->pos = pos;
	updateModel();
}

void TextRenderer::align(bool centered)
{
	this->centered = centered;
	updateModel();
}

void TextRenderer::render() const
{
	glActiveTexture(GL_TEXTURE0);

	shader->use();
	texture->use();

	// set uniforms
	glUniform4fv(glGetUniformLocation(shader->id(), "inColor"), 1, &color[0]);
	glUniform2i(glGetUniformLocation(shader->id(), "charSize"), charSize.x * fontSize, charSize.y * fontSize);
	glUniformMatrix4fv(glGetUniformLocation(shader->id(), "MV"), 1, GL_FALSE, &model[0][0]);

	glBindVertexArray(VAO);

	glDrawArrays(GL_POINTS, 0, text.length());

	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::updateModel()
{
	model = glm::mat4(1.0f);

	int xPos = pos.x;
	int yPos = pos.y;
	if (centered)
	{
		xPos -= text.length() * charSize.x * fontSize / 2;
		yPos -= charSize.y * fontSize / 2;
	}

	model = glm::translate(model, glm::vec3(xPos, yPos, 0.0f));

	//model = glm::translate(model, glm::vec3(0.5f * w, 0.5f * h, 0.0f));
	//model = glm::rotate(model, radians, glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::translate(model, glm::vec3(-0.5f * w, -0.5f * h, 0.0f));

	//model = glm::scale(model, glm::vec3(w, h, 1.0f));
}

void TextRenderer::updateCharVBO()
{
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, charVBO);

	glBufferData(GL_ARRAY_BUFFER, text.length() * sizeof(GLbyte), text.c_str(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void TextRenderer::init()
{
	cleanup();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &charVBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, charVBO);

	glBufferData(GL_ARRAY_BUFFER, text.length() * sizeof(GLbyte), text.c_str(), GL_STATIC_DRAW);
	glVertexAttribIPointer(0, 1, GL_BYTE, 1 * sizeof(GLbyte), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void TextRenderer::move(TextRenderer& other)
{
	text = other.text;

	charSize = other.charSize;
	fontSize = other.fontSize;
	pos = other.pos;
	color = other.color;
	model = other.model;
	centered = other.centered;

	texture = other.texture;
	shader = other.shader;

	VAO = other.VAO;
	other.VAO = 0;

	charVBO = other.charVBO;
}

void TextRenderer::cleanup()
{
	glBindVertexArray(this->VAO);
	glDeleteBuffers(1, &this->charVBO);
	glDeleteVertexArrays(1, &this->VAO);
}
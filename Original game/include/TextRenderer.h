#pragma once
#include <glad/gl.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <string>

class Texture;
class Shader;

class TextRenderer
{
public:

	TextRenderer(const Texture* texture, const Shader* shader);
	TextRenderer();
	~TextRenderer();
	TextRenderer(TextRenderer&& other) noexcept;
	TextRenderer& operator=(TextRenderer&& other) noexcept;

	void setTexture(const Texture* t);
	void setShader(const Shader* s);

	// must call before rendering if the empty `TextRenderer()` constructor is used.
	void init();

	void setCharSize(int w, int h);
	void setCharSize(const glm::ivec2 size);
	int getCharWidth();
	int getCharHeight();

	void setFontSize(int size);
	int getFontSize();

	void setColor(float r, float g, float b, float a = 1.0f);
	void setColor(const glm::vec4& color);
	void setColor(const glm::vec3& color);

	void setText(std::string_view text);

	void setPos(int x, int y);
	void setPos(const glm::ivec2& pos);
	void align(bool centered);

	void render() const;

private:

	void updateModel();
	void updateCharVBO();

	void move(TextRenderer& other);
	void cleanup();

	std::string text;

	glm::ivec2 charSize{ 8, 11 };
	int fontSize = 1;
	glm::ivec2 pos{ 0, 0 };
	glm::vec4 color{ 1, 1, 1, 1 };
	glm::mat4 model{ 1 };
	bool centered = false;

	const Texture* texture = nullptr;
	const Shader* shader = nullptr;
	GLuint VAO = 0;
	GLuint charVBO = 0;
};
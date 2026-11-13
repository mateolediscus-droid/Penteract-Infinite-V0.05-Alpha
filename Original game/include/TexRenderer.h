#pragma once
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

class Texture;
class Shader;

class TexRenderer
{
protected:
	glm::vec2 pos{ 0 };
	glm::vec2 scale{ 1 };
	float angle = 0;
	glm::vec4 clip{ 0.0f, 0.0f, 1.0f, 1.0f };
	glm::vec2 rotOrigin{ 0.5f };
	glm::vec2 scaleOrigin{ 0.5f };
	glm::mat4 model{ 1 };
	bool updateModel = true;
	const Texture* texture = nullptr;
	const Shader* shader = nullptr;
public:

	TexRenderer(const Texture* texture = nullptr, const Shader* shader = nullptr);
	const Texture* getTexture() const { return texture; }
	void setTexture(const Texture* texture) { this->texture = texture; }
	const Shader* getShader() const { return shader; }
	void setShader(const Shader* shader) { this->shader = shader; }
	void setPos(const glm::vec2& pos);
	void setPos(float x, float y) { setPos({ x, y }); }
	void setClip(const glm::vec4& clip);
	void setScale(const glm::vec2& scale);
	void setScale(float x, float y) { setScale({ x, y }); }
	void setAngle(float angle);
	void setRotOrigin(const glm::vec2& rotOrigin);
	void setRotOrigin(float x, float y) { setRotOrigin({ x, y }); }
	void setScaleOrigin(const glm::vec2& scaleOrigin);
	void setScaleOrigin(float x, float y) { setScaleOrigin({ x, y }); }
	glm::vec2 getPos() const { return pos; }
	glm::vec4 getClip() const { return clip; }
	glm::vec2 getScale() const { return scale; }
	float getAngle() const { return angle; }
	glm::vec2 getRotOrigin() const { return rotOrigin; }
	glm::vec2 getScaleOrigin() const { return scaleOrigin; }
	void render();
};
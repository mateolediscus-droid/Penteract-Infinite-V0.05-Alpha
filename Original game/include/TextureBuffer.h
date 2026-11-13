#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include "Math5D.h"
#include <format>

enum TextureFormat : int
{
	R8i,
	RG8i,
	RGB8i,
	RGBA8i,
	R8u,
	RG8u,
	RGB8u,
	RGBA8u,
	R16i,
	RG16i,
	RGB16i,
	RGBA16i,
	R16u,
	RG16u,
	RGB16u,
	RGBA16u,
	R32i,
	RG32i,
	RGB32i,
	RGBA32i,
	R32u,
	RG32u,
	RGB32u,
	RGBA32u,
	R32f,
	RG32f,
	RGB32f,
	RGBA32f,
	R11f_G11f_B10f
};

extern GLenum glType(TextureFormat type);
extern GLenum glCompType(TextureFormat type);
extern GLenum glFormat(TextureFormat type);

class TextureBuffer
{
public:
	enum WrapMode : int
	{
		REPEAT,
		CLAMP,
		MIRROR
	};

	enum class MinFilter : int
	{
		NEAREST,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_NEAREST,
		LINEAR_MIPMAP_LINEAR,
	};

	enum class MagFilter : int
	{
		NEAREST,
		LINEAR
	};

private:
	uint32_t ID = NULL;
	size_t x = 0, y = 0, z = 0;
	int dimensions = 1;
	TextureFormat type = R8i;
	uint64_t handle = NULL;
	int mipLevels = 1;

	void init(size_t x, TextureFormat type, const void* data = nullptr, int mipLevels = 1);
	void init(size_t x, size_t y, TextureFormat type, const void* data = nullptr, int mipLevels = 1);
	void init(size_t x, size_t y, size_t z, TextureFormat type, const void* data = nullptr, int mipLevels = 1);

	static GLenum glMinFilter(MinFilter mode);
	static GLenum glMagFilter(MagFilter mode);
	static GLenum glWrapMode(WrapMode mode);

public:
	TextureBuffer() {}
	TextureBuffer(TextureBuffer&& other) noexcept;
	TextureBuffer& operator=(TextureBuffer&& other) noexcept;
	TextureBuffer(size_t x, TextureFormat type, const void* data = nullptr, int mipLevels = 1);
	TextureBuffer(size_t x, size_t y, TextureFormat type, const void* data = nullptr, int mipLevels = 1);
	TextureBuffer(size_t x, size_t y, size_t z, TextureFormat type, const void* data = nullptr, int mipLevels = 1);

	void uploadData(size_t x, const void* data = nullptr, int mipLevel = 0);
	void uploadData(size_t x, size_t y, const void* data = nullptr, int mipLevel = 0);
	void uploadData(size_t x, size_t y, size_t z, const void* data = nullptr, int mipLevel = 0);

	void uploadDataOffset(size_t xOffset, size_t x, const void* data = nullptr, int mipLevel = 0);
	void uploadDataOffset(size_t xOffset, size_t yOffset, size_t x, size_t y, const void* data = nullptr, int mipLevel = 0);
	void uploadDataOffset(size_t xOffset, size_t yOffset, size_t zOffset, size_t x, size_t y, size_t z, const void* data = nullptr, int mipLevel = 0);

	// might clear the data! use with caution!
	void fit(size_t x);
	void fit(size_t x, size_t y);
	void fit(size_t x, size_t y, size_t z);

	// might clear the data! use with caution!
	void resize(size_t x);
	void resize(size_t x, size_t y);
	void resize(size_t x, size_t y, size_t z);

	void cleanup();

	void generateMipmaps();
	void setFilters(MinFilter min, MagFilter mag);
	void setWrapMode(WrapMode x);
	void setWrapMode(WrapMode x, WrapMode y);
	void setWrapMode(WrapMode x, WrapMode y, WrapMode z);

	uint32_t id() const
	{
		return ID;
	}

	void bind(uint32_t unit) const
	{
		if (ID)
		{
			glBindTextureUnit(unit, ID);
		}
	}

	uint64_t getHandle() const
	{
		return handle;
	}

	TextureFormat getType() const
	{
		return type;
	}
	int getDimensions() const
	{
		return dimensions;
	}
	// x
	size_t width() const
	{
		return x;
	}
	// y
	size_t height() const
	{
		return y;
	}
	// z
	size_t depth() const
	{
		return z;
	}
	size_t getX() const
	{
		return x;
	}
	size_t getY() const
	{
		return y;
	}
	size_t getZ() const
	{
		return z;
	}

	~TextureBuffer();
};
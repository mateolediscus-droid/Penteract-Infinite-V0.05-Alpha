#include "TextureBuffer.h"

TextureBuffer::TextureBuffer(size_t x, TextureFormat type, const void* data, int mipLevels)
{
	init(x, type, data, mipLevels);
}

TextureBuffer::TextureBuffer(size_t x, size_t y, TextureFormat type, const void* data, int mipLevels)
{
	init(x, y, type, data, mipLevels);
}

TextureBuffer::TextureBuffer(size_t x, size_t y, size_t z, TextureFormat type, const void* data, int mipLevels)
{
	init(x, y, z, type, data, mipLevels);
}

TextureBuffer::~TextureBuffer()
{
	cleanup();
}

void TextureBuffer::uploadData(size_t x, const void* data, int mipLevel)
{
	uploadDataOffset(0, x, data, mipLevel);
}

void TextureBuffer::uploadData(size_t x, size_t y, const void* data, int mipLevel)
{
	uploadDataOffset(0, 0, x, y, data, mipLevel);
}

void TextureBuffer::uploadData(size_t x, size_t y, size_t z, const void* data, int mipLevel)
{
	uploadDataOffset(0, 0, 0, x, y, z, data, mipLevel);
}

void TextureBuffer::uploadDataOffset(size_t xOffset, size_t x, const void* data, int mipLevel)
{
	if (dimensions != 1)
	{
		throw std::runtime_error(std::format("Tried uploading 1D data into a {}D texture!", dimensions));
	}

	if (data == nullptr) return;

	if (mipLevel != 0)
	{
		int mipX = this->x / pow(2, mipLevel);
		if (xOffset + x > mipX)
		{
			throw std::runtime_error("Can't resize the texture buffer when uploading to non-base mip level!");
		}
	}
	fit(xOffset + x);

	glTextureSubImage1D(ID, mipLevel, xOffset, x, glFormat(type), glCompType(type), data);
}

void TextureBuffer::uploadDataOffset(size_t xOffset, size_t yOffset, size_t x, size_t y, const void* data, int mipLevel)
{
	if (dimensions != 2)
	{
		throw std::runtime_error(std::format("Tried uploading 2D data into a {}D texture!", dimensions));
	}

	if (data == nullptr) return;

	if (mipLevel != 0)
	{
		int mipX = this->x / pow(2, mipLevel);
		int mipY = this->y / pow(2, mipLevel);
		if (xOffset + x > mipX || yOffset + y > mipY)
		{
			throw std::runtime_error("Can't resize the texture buffer when uploading to non-base mip level!");
		}
	}
	fit(xOffset + x, yOffset + y);

	glTextureSubImage2D(ID, mipLevel, xOffset, yOffset, x, y, glFormat(type), glCompType(type), data);
}

void TextureBuffer::uploadDataOffset(size_t xOffset, size_t yOffset, size_t zOffset, size_t x, size_t y, size_t z, const void* data, int mipLevel)
{
	if (dimensions != 3)
	{
		throw std::runtime_error(std::format("Tried uploading 3D data into a {}D texture!", dimensions));
	}

	if (data == nullptr) return;

	if (mipLevel != 0)
	{
		int mipX = this->x / pow(2, mipLevel);
		int mipY = this->y / pow(2, mipLevel);
		int mipZ = this->z / pow(2, mipLevel);
		if (xOffset + x > mipX || yOffset + y > mipY || zOffset + z > mipZ)
		{
			throw std::runtime_error("Can't resize the texture buffer when uploading to non-base mip level!");
		}
	}
	fit(xOffset + x, yOffset + y, zOffset + z);

	glTextureSubImage3D(ID, mipLevel, xOffset, yOffset, zOffset, x, y, z, glFormat(type), glCompType(type), data);
}

void TextureBuffer::fit(size_t x)
{
	if (x > this->x)
	{
		init(x, type, nullptr, mipLevels);
	}
}

void TextureBuffer::fit(size_t x, size_t y)
{
	if (x > this->x || y > this->y)
	{
		init(x, y, type, nullptr, mipLevels);
	}
}

void TextureBuffer::fit(size_t x, size_t y, size_t z)
{
	if (x > this->x || y > this->y || z > this->z)
	{
		init(x, y, z, type, nullptr, mipLevels);
	}
}

void TextureBuffer::resize(size_t x)
{
	if (x != this->x)
	{
		init(x, type, nullptr, mipLevels);
	}
}

void TextureBuffer::resize(size_t x, size_t y)
{
	if (x != this->x || y != this->y)
	{
		init(x, y, type, nullptr, mipLevels);
	}
}

void TextureBuffer::resize(size_t x, size_t y, size_t z)
{
	if (x != this->x || y != this->y || z != this->z)
	{
		init(x, y, z, type, nullptr, mipLevels);
	}
}

void TextureBuffer::generateMipmaps()
{
	glGenerateMipmap(ID);
}
void TextureBuffer::setFilters(MinFilter min, MagFilter mag)
{
	glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, glMinFilter(min));
	glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, glMagFilter(mag));
}
void TextureBuffer::setWrapMode(WrapMode x)
{
	glTextureParameteri(ID, GL_TEXTURE_WRAP_S, glWrapMode(x));
}
void TextureBuffer::setWrapMode(WrapMode x, WrapMode y)
{
	glTextureParameteri(ID, GL_TEXTURE_WRAP_S, glWrapMode(x));
	glTextureParameteri(ID, GL_TEXTURE_WRAP_T, glWrapMode(y));
}
void TextureBuffer::setWrapMode(WrapMode x, WrapMode y, WrapMode z)
{
	glTextureParameteri(ID, GL_TEXTURE_WRAP_S, glWrapMode(x));
	glTextureParameteri(ID, GL_TEXTURE_WRAP_T, glWrapMode(y));
	glTextureParameteri(ID, GL_TEXTURE_WRAP_R, glWrapMode(z));
}

void TextureBuffer::cleanup()
{
	if (handle)
	{
		glMakeTextureHandleNonResidentARB(handle);
		handle = NULL;
	}
	if (ID)
	{
		glDeleteTextures(1, &ID);
		ID = NULL;
		dimensions = 1;
		type = R8i;
		x = 0;
		y = 0;
		z = 0;
		mipLevels = 1;
	}
}

void TextureBuffer::init(size_t x, TextureFormat type, const void* data, int mipLevels)
{
	cleanup();

	mipLevels = std::min(mipLevels, (int)floor(log2(x)) + 1);
	mipLevels = std::max(mipLevels, 1);
	this->dimensions = 1;
	this->x = x;
	this->type = type;
	this->mipLevels = mipLevels;
	
	glCreateTextures(GL_TEXTURE_1D, 1, &ID);

	glTextureStorage1D(ID, mipLevels, glType(type), x);

	setWrapMode(WrapMode::CLAMP);
	setFilters(MinFilter::NEAREST, MagFilter::NEAREST);

	handle = glGetTextureHandleARB(ID);
	glMakeTextureHandleResidentARB(handle);

	if (data != nullptr)
	{
		uploadData(x, data);
	}
}

void TextureBuffer::init(size_t x, size_t y, TextureFormat type, const void* data, int mipLevels)
{
	cleanup();

	mipLevels = std::min(mipLevels, (int)floor(log2(std::max(x, y))) + 1);
	mipLevels = std::max(mipLevels, 1);
	this->dimensions = 2;
	this->x = x;
	this->y = y;
	this->type = type;
	this->mipLevels = mipLevels;

	glCreateTextures(GL_TEXTURE_2D, 1, &ID);

	glTextureStorage2D(ID, mipLevels, glType(type), x, y);

	setWrapMode(WrapMode::CLAMP, WrapMode::CLAMP);
	setFilters(MinFilter::NEAREST, MagFilter::NEAREST);

	handle = glGetTextureHandleARB(ID);
	glMakeTextureHandleResidentARB(handle);

	if (data != nullptr)
	{
		uploadData(x, y, data);
	}
}

void TextureBuffer::init(size_t x, size_t y, size_t z, TextureFormat type, const void* data, int mipLevels)
{
	cleanup();

	mipLevels = std::min(mipLevels, (int)floor(log2(std::max(x, std::max(y, z)))) + 1);
	mipLevels = std::max(mipLevels, 1);
	this->dimensions = 3;
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
	this->mipLevels = mipLevels;

	glCreateTextures(GL_TEXTURE_3D, 1, &ID);

	glTextureStorage3D(ID, mipLevels, glType(type), x, y, z);

	setWrapMode(WrapMode::CLAMP, WrapMode::CLAMP, WrapMode::CLAMP);
	setFilters(MinFilter::NEAREST, MagFilter::NEAREST);

	handle = glGetTextureHandleARB(ID);
	glMakeTextureHandleResidentARB(handle);

	if (data != nullptr)
	{
		uploadData(x, y, z, data);
	}
}

TextureBuffer::TextureBuffer(TextureBuffer&& other) noexcept
{
	this->ID = other.ID;
	this->dimensions = other.dimensions;
	this->type = other.type;
	this->x = other.x;
	this->y = other.y;
	this->z = other.z;
	this->handle = other.handle;
	this->mipLevels = other.mipLevels;

	other.ID = NULL;
	other.dimensions = 1;
	other.type = R8i;
	other.x = 0;
	other.y = 0;
	other.z = 0;
	other.handle = NULL;
	other.mipLevels = 1;
}
TextureBuffer& TextureBuffer::operator=(TextureBuffer&& other) noexcept
{
	if (this != &other)
	{
		this->ID = other.ID;
		this->dimensions = other.dimensions;
		this->type = other.type;
		this->x = other.x;
		this->y = other.y;
		this->z = other.z;
		this->handle = other.handle;
		this->mipLevels = other.mipLevels;

		other.ID = NULL;
		other.dimensions = 1;
		other.type = R8i;
		other.x = 0;
		other.y = 0;
		other.z = 0;
		other.handle = NULL;
		other.mipLevels = 1;
	}

	return *this;
}

GLenum glType(TextureFormat type)
{
	switch (type)
	{
	case R8i:
		return GL_R8I;
	case RG8i:
		return GL_RG8I;
	case RGB8i:
		return GL_RGB8I;
	case RGBA8i:
		return GL_RGBA8I;
	case R8u:
		return GL_R8UI;
	case RG8u:
		return GL_RG8UI;
	case RGB8u:
		return GL_RGB8UI;
	case RGBA8u:
		return GL_RGBA8UI;
	case R16i:
		return GL_R16I;
	case RG16i:
		return GL_RG16I;
	case RGB16i:
		return GL_RGB16I;
	case RGBA16i:
		return GL_RGBA16I;
	case R16u:
		return GL_R16UI;
	case RG16u:
		return GL_RG16UI;
	case RGB16u:
		return GL_RGB16UI;
	case RGBA16u:
		return GL_RGBA16UI;
	case R32i:
		return GL_R32I;
	case RG32i:
		return GL_RG32I;
	case RGB32i:
		return GL_RGB32I;
	case RGBA32i:
		return GL_RGBA32I;
	case R32u:
		return GL_R32UI;
	case RG32u:
		return GL_RG32UI;
	case RGB32u:
		return GL_RGB32UI;
	case RGBA32u:
		return GL_RGBA32UI;
	case R32f:
		return GL_R32F;
	case RG32f:
		return GL_RG32F;
	case RGB32f:
		return GL_RGB32F;
	case RGBA32f:
		return GL_RGBA32F;
	case R11f_G11f_B10f:
		return GL_R11F_G11F_B10F;
	}
	return GL_R8I;
}

GLenum glFormat(TextureFormat type)
{
	switch (type)
	{
	case R8i:
	case R8u:
	case R16i:
	case R16u:
	case R32i:
	case R32u:
		return GL_RED_INTEGER;
	case R32f:
		return GL_RED;
	case RG8i:
	case RG8u:
	case RG16i:
	case RG16u:
	case RG32i:
	case RG32u:
		return GL_RG_INTEGER;
	case RG32f:
		return GL_RG;
	case RGB8i:
	case RGB8u:
	case RGB16i:
	case RGB16u:
	case RGB32i:
	case RGB32u:
		return GL_RGB_INTEGER;
	case RGB32f:
	case R11f_G11f_B10f:
		return GL_RGB;
	case RGBA8i:
	case RGBA8u:
	case RGBA16i:
	case RGBA16u:
	case RGBA32i:
	case RGBA32u:
		return GL_RGBA_INTEGER;
	case RGBA32f:
		return GL_RGBA;
	}
	return GL_RED;
}

GLenum glCompType(TextureFormat type)
{
	switch (type)
	{
	case R8i:
	case RG8i:
	case RGB8i:
	case RGBA8i:
		return GL_BYTE;
	case R8u:
	case RG8u:
	case RGB8u:
	case RGBA8u:
		return GL_UNSIGNED_BYTE;
	case R16i:
	case RG16i:
	case RGB16i:
	case RGBA16i:
		return GL_SHORT;
	case R16u:
	case RG16u:
	case RGB16u:
	case RGBA16u:
		return GL_UNSIGNED_SHORT;
	case R32i:
	case RG32i:
	case RGB32i:
	case RGBA32i:
		return GL_INT;
	case R32u:
	case RG32u:
	case RGB32u:
	case RGBA32u:
		return GL_UNSIGNED_INT;
	case R32f:
	case RG32f:
	case RGB32f:
	case RGBA32f:
	case R11f_G11f_B10f:
		return GL_FLOAT;
	}
	return GL_BYTE;
}

GLenum TextureBuffer::glMinFilter(MinFilter mode)
{
	switch (mode)
	{
	case TextureBuffer::MinFilter::NEAREST:
		return GL_NEAREST;
	case TextureBuffer::MinFilter::LINEAR:
		return GL_LINEAR;
	case TextureBuffer::MinFilter::NEAREST_MIPMAP_NEAREST:
		return GL_NEAREST_MIPMAP_LINEAR;
	case TextureBuffer::MinFilter::NEAREST_MIPMAP_LINEAR:
		return GL_NEAREST_MIPMAP_NEAREST;
	case TextureBuffer::MinFilter::LINEAR_MIPMAP_NEAREST:
		return GL_LINEAR_MIPMAP_NEAREST;
	case TextureBuffer::MinFilter::LINEAR_MIPMAP_LINEAR:
		return GL_LINEAR_MIPMAP_LINEAR;
	}
	return GL_NEAREST;
}
GLenum TextureBuffer::glMagFilter(MagFilter mode)
{
	switch (mode)
	{
	case TextureBuffer::MagFilter::NEAREST:
		return GL_NEAREST;
	case TextureBuffer::MagFilter::LINEAR:
		return GL_LINEAR;
	}
	return GL_NEAREST;
}
GLenum TextureBuffer::glWrapMode(WrapMode mode)
{
	switch (mode)
	{
	case TextureBuffer::REPEAT:
		return GL_REPEAT;
	case TextureBuffer::CLAMP:
		return GL_CLAMP_TO_EDGE;
	case TextureBuffer::MIRROR:
		return GL_MIRRORED_REPEAT;
	}
	return GL_REPEAT;
}

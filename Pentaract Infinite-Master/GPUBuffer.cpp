#include "GPUBuffer.h"

void GPUBuffer::init(size_t size, const void* data)
{
	cleanup();

	this->size = size;

	glCreateBuffers(1, &ID);
	glNamedBufferStorage(ID, this->size, nullptr, GL_DYNAMIC_STORAGE_BIT);

	if (data != nullptr)
	{
		uploadData(size, data);
	}
}

GPUBuffer::GPUBuffer(size_t size, const void* data)
{
	init(size, data);
}

void GPUBuffer::uploadData(size_t offset, size_t size, const void* data)
{
	if (data == nullptr) return;

	fit(offset + size);

	glNamedBufferSubData(ID, offset, size, data);
}

void GPUBuffer::fit(size_t size)
{
	if (size > this->size)
	{
		init(size, nullptr);
	}
}

void GPUBuffer::resize(size_t size)
{
	if (size != this->size)
	{
		init(size, nullptr);
	}
}

void GPUBuffer::cleanup()
{
	if (ID)
	{
		glDeleteBuffers(1, &ID);
		ID = NULL;
		size = 0;
	}
}

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
{
	this->ID = other.ID;
	this->size = other.size;

	other.ID = NULL;
	other.size = 0;
}
GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
{
	if (this != &other)
	{
		this->ID = other.ID;
		this->size = other.size;

		other.ID = NULL;
		other.size = 0;
	}

	return *this;
}

GPUBuffer::~GPUBuffer()
{
	cleanup();
}

GLenum GPUBuffer::glBufferType(BufferType type)
{
	switch (type)
	{
	case SHADER_STORAGE_BUFFER:
		return GL_SHADER_STORAGE_BUFFER;
	case UNIFORM_BUFFER:
		return GL_UNIFORM_BUFFER;
	}
	return GL_SHADER_STORAGE_BUFFER;
}

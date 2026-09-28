#pragma once

#include "Radiant/Renderer/UniformBuffer.h"

namespace Radiant {

	/** GL uniform buffer, bound once to its binding point (glBindBufferBase) at construction. Owns the GL buffer handle. */
	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~OpenGLUniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
	private:
		uint32_t m_RendererID = 0;
	};
}
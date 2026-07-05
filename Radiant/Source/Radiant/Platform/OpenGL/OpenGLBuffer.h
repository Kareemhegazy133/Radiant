#pragma once

#include "Radiant/Renderer/Buffer.h"

namespace Radiant {

	/**
	 * GL vertex buffer. The size-only constructor allocates GL_DYNAMIC_DRAW
	 * storage for per-frame streaming; the data constructor uploads immutable
	 * GL_STATIC_DRAW data. Owns the GL buffer handle (deleted in the
	 * destructor).
	 */
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer(float* vertices, uint32_t size);
		virtual ~OpenGLVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetData(const void* data, uint32_t size) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

	private:
		uint32_t m_RendererID;
		BufferLayout m_Layout;
	};

	/** GL index buffer (GL_ELEMENT_ARRAY_BUFFER, 32-bit indices). Owns the GL buffer handle. */
	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~OpenGLIndexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual uint32_t GetCount() const { return m_Count; }
	private:
		uint32_t m_RendererID;
		uint32_t m_Count;
	};

}
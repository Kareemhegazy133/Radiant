#include "Radiant/rdpch.h"
#include "Buffer.h"

#include "Renderer.h"

#include "Radiant/Platform/OpenGL/OpenGLBuffer.h"

namespace Radiant {

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::API::OpenGL:
				return Ref<OpenGLVertexBuffer>::Create(size);
		}
		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::API::OpenGL:
				return Ref<OpenGLVertexBuffer>::Create(vertices, size);
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return Ref<OpenGLIndexBuffer>::Create(indices, count);
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
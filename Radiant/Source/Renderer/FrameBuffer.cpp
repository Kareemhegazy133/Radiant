#include "rdpch.h"
#include "Framebuffer.h"

#include "Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace Radiant {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::API::OpenGL:
				return CreateRef<OpenGLFramebuffer>(spec);
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
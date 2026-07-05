#include "Radiant/rdpch.h"
#include "Framebuffer.h"

#include "Radiant/Renderer/Renderer.h"
#include "Radiant/Platform/OpenGL/OpenGLFramebuffer.h"

namespace Radiant {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::API::OpenGL:
				return Ref<OpenGLFramebuffer>::Create(spec);
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
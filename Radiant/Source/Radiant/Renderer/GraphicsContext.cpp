#include "Radiant/rdpch.h"
#include "Radiant/Renderer/GraphicsContext.h"

#include "Radiant/Renderer/Renderer.h"
#include "Radiant/Platform/OpenGL/OpenGLContext.h"

namespace Radiant {

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
				return nullptr;
			case RendererAPI::API::OpenGL:
				return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
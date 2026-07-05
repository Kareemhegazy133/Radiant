#include "Radiant/rdpch.h"
#include "Texture.h"

#include "Renderer.h"
#include "Radiant/Platform/OpenGL/OpenGLTexture.h"

namespace Radiant {

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return Ref<OpenGLTexture2D>::Create(specification, data);
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
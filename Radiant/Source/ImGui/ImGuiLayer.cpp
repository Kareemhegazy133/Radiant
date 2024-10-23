#include "rdpch.h"
#include "ImGuiLayer.h"

#include <imgui.h>

#include "Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLImGuiLayer.h"

#include "Renderer/RendererAPI.h"

namespace Radiant {

	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return new OpenGLImGuiLayer();
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void ImGuiLayer::SetDarkThemeColors()
	{

	}

}
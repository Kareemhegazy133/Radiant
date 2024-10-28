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

	ImGuiLayer* ImGuiLayer::Create(const FontConfiguration& config)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			RADIANT_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return new OpenGLImGuiLayer(config);
		}

		RADIANT_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

    void ImGuiLayer::SetFont(const FontConfiguration& config)
    {
        ImGuiIO& io = ImGui::GetIO();

        // Clear any previously loaded fonts
        io.Fonts->Clear();

        std::string fontPathStr = config.FilePath.string();
        io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), config.Size);

        // Build the atlas
        unsigned char* tex_pixels = nullptr;
        int tex_width, tex_height;
        io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);
    }
}
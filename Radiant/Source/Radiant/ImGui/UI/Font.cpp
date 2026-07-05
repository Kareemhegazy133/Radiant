#include "Radiant/rdpch.h"
#include "Font.h"

#include <imgui.h>

#include "Radiant/Asset/AssetManager.h"

namespace Radiant {

	Font::Font(const FontConfiguration& config)
	{
		m_Name = config.FilePath.stem().string();

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

	void Font::Init(const FontConfiguration& config)
	{
		s_DefaultFont = Ref<Font>::Create(config);
	}

	void Font::Shutdown()
	{
		s_DefaultFont.Reset();
	}

	Ref<Font> Font::GetDefaultFont()
	{
		RADIANT_ASSERT(s_DefaultFont, "Font: No default font has been initialized");
		return s_DefaultFont;
	}

}
#include "Radiant/rdpch.h"
#include "Font.h"

#include <imgui.h>

#include "Radiant/Asset/AssetManager.h"

namespace Radiant {

	Font::Font(const FontConfiguration& config)
	{
		m_Name = config.FilePath.stem().string();

		ImGuiIO& io = ImGui::GetIO();

		// The ImGui atlas is global: clearing evicts every previously loaded font,
		// so only the font constructed last can render (see class doc — RAD-47).
		io.Fonts->Clear();

		std::string fontPathStr = config.FilePath.string();
		io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), config.Size);

		// GetTexDataAsRGBA32 forces the atlas build now rather than during the
		// backend's next NewFrame; the pixel data itself is not used here.
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
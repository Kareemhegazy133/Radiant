#pragma once

#include <filesystem>

#include "Radiant/ECS/Components.h"

namespace Radiant {

	/** Source .ttf path and glyph size in pixels for the ImGui atlas build. */
	struct FontConfiguration
	{
		std::filesystem::path FilePath = "Assets/Fonts/Roboto/Roboto-Regular.ttf";
		float Size = 18.0f;
	};

	/**
	 * TRANSITIONAL font wrapper over ImGui's built-in atlas — the stopgap left
	 * after the MSDF text pipeline was removed; RAD-47 revives MSDF and re-wires
	 * Font as a managed asset type (which is why the AssetType hooks below are
	 * commented out and no FontSerializerAPI is registered). Constructing a Font
	 * clears and rebuilds the GLOBAL ImGui font atlas, so only the most recently
	 * created Font renders correctly — treat the static default (Init/Shutdown/
	 * GetDefaultFont) as the single live instance.
	 */
	class Font : public Asset
	{
	public:
		Font(const FontConfiguration& config =  FontConfiguration());
		virtual ~Font() = default;

		static void Init(const FontConfiguration& config);
		static void Shutdown();

		/** Asserts if Font::Init has not run. */
		static Ref<Font> GetDefaultFont();

		/*static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }*/

		const std::string& GetName() const { return m_Name; }

	private:
		inline static Ref<Font> s_DefaultFont;
		std::string m_Name;
	};


}
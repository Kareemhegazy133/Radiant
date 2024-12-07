#pragma once

#include <filesystem>

#include "ECS/Components.h"

namespace Radiant {

	struct FontConfiguration
	{
		std::filesystem::path FilePath = "Assets/Fonts/Roboto/Roboto-Regular.ttf";
		float Size = 18.0f;
	};

	class Font : public Asset
	{
	public:
		Font(const FontConfiguration& config =  FontConfiguration());
		virtual ~Font() = default;

		static void Init(const FontConfiguration& config);
		static void Shutdown();

		static Ref<Font> GetDefaultFont();

		/*static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }*/

		const std::string& GetName() const { return m_Name; }

	private:
		inline static Ref<Font> s_DefaultFont;
		std::string m_Name;
	};


}
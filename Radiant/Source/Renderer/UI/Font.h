#pragma once

#include <filesystem>

#include "Renderer/Texture.h"
#include "ECS/Components.h"

namespace Radiant {

	struct MSDFData;

	class Font : public Asset
	{
	public:
		Font(const std::filesystem::path& filepath);
		Font(const std::string& name, Buffer buffer);
		virtual ~Font();

		static Ref<Font> Init(const std::filesystem::path& filepath = "Assets/Fonts/OpenSans/OpenSans-Regular.ttf");
		static void Shutdown();

		Ref<Texture2D> GetFontAtlas() const { return m_TextureAtlas; }
		const MSDFData* GetMSDFData() const { return m_MSDFData; }

		static Ref<Font> GetDefaultFont();
		static Ref<Font> GetFontAssetForTextComponent(TextComponent& textComponent);

		static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		const std::string& GetName() const { return m_Name; }

	private:
		void CreateAtlas(Buffer buffer);

	private:
		std::string m_Name;
		Ref<Texture2D> m_TextureAtlas;
		MSDFData* m_MSDFData = nullptr;

	private:
		inline static Ref<Font> s_DefaultFont;
	};


}
#pragma once

#include <filesystem>

#include "Core/Base.h"
#include "Renderer/Texture.h"

namespace Radiant {

	struct MSDFData;

	class Font : public Asset
	{
	public:
		Font(const std::filesystem::path& font);
		Font(const std::string& name, Buffer buffer);
		virtual ~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

		static Ref<Font> GetDefault();

		static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		const std::string& GetName() const { return m_Name; }

	private:
		void CreateAtlas(Buffer buffer);
	private:
		std::string m_Name;
		MSDFData* m_Data;
		Ref<Texture2D> m_AtlasTexture;
	};

}
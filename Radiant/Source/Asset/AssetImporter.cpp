#include "rdpch.h"
#include "AssetImporter.h"

#include <map>

#include "TextureImporter.h"
#include "LevelImporter.h"


namespace Radiant {

	using AssetImportFunction = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;

	static std::map<AssetType, AssetImportFunction> s_AssetImportFunctions =
	{
		{ AssetType::Level, LevelImporter::ImportLevel },
		{ AssetType::Texture2D, TextureImporter::ImportTexture2D }

	};

	Ref<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (s_AssetImportFunctions.find(metadata.Type) == s_AssetImportFunctions.end())
		{
			RADIANT_ERROR("No importer available for asset type: {}", (uint16_t)metadata.Type);
			return nullptr;
		}

		return s_AssetImportFunctions.at(metadata.Type)(handle, metadata);
	}

}
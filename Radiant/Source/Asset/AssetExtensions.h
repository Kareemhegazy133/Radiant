#pragma once

#include <unordered_map>

#include "AssetTypes.h"

namespace Radiant {

	inline static std::unordered_map<std::filesystem::path, AssetType> s_AssetExtensionMap =
	{
		// Radiant types
		{ ".level", AssetType::Level },

		// Textures
		{ ".png", AssetType::Texture2D },
		{ ".jpg", AssetType::Texture2D },
		{ ".jpeg", AssetType::Texture2D }
	};

	static AssetType GetAssetTypeFromFileExtension(const std::filesystem::path& extension)
	{
		if (s_AssetExtensionMap.find(extension) == s_AssetExtensionMap.end())
		{
			RADIANT_WARN("Could not find AssetType for {0}", extension.string());
			return AssetType::None;
		}

		return s_AssetExtensionMap.at(extension);
	}

}
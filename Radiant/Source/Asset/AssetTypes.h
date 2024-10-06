#pragma once

#include "Core/Assert.h"

namespace Radiant {

	enum class AssetType : uint8_t
	{
		None = 0,
		Level,
		Texture2D
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")                return AssetType::None;
			if (assetType == "Level")               return AssetType::Level;
			if (assetType == "Texture2D")           return AssetType::Texture2D;

			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
			case AssetType::None:                return "None";
			case AssetType::Level:               return "Level";
			case AssetType::Texture2D:             return "Texture2D";
			}

			RADIANT_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}
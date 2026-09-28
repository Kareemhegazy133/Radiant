#pragma once

#include "Radiant/Core/Assert.h"

namespace Radiant {

	/**
	 * Discriminates every loadable asset kind. Persisted by NAME (see
	 * Utils::AssetTypeToString) in .rdar registry files, so renaming an entry is
	 * a data-format change. Adding a type also requires an AssetSerializerAPI
	 * registration in AssetSerializer::Init and an extension mapping in
	 * AssetExtensions.h.
	 */
	enum class AssetType : uint8_t
	{
		None = 0,
		Level,
		Texture2D
	};

	namespace Utils {

		/** Returns AssetType::None for unrecognized names — callers treat None as "skip this entry". */
		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")                return AssetType::None;
			if (assetType == "Level")               return AssetType::Level;
			if (assetType == "Texture2D")           return AssetType::Texture2D;

			return AssetType::None;
		}

		/** Asserts on values missing from the switch — extend it when adding an AssetType. */
		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
			case AssetType::None:                return "None";
			case AssetType::Level:               return "Level";
			case AssetType::Texture2D:           return "Texture2D";
			}

			RADIANT_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}
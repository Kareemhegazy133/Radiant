#pragma once

#include <filesystem>

#include "Asset.h"

namespace Radiant {

	/**
	 * A registry record: everything the AssetManager knows about an asset without
	 * loading it. This is the only place a file path is attached to a handle —
	 * runtime code never sees paths. A default-constructed instance (Handle 0,
	 * Type None) is the "not found" value returned by metadata lookups.
	 */
	struct AssetMetadata
	{
		AssetHandle Handle = 0;
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath;

		// Valid = has a recognized type; does NOT imply the handle is registered.
		bool IsValid() const { return Type != AssetType::None; }
	};
}
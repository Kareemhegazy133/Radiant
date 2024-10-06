#pragma once

#include <filesystem>

#include "Asset.h"

namespace Radiant {

	struct AssetMetadata
	{
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath;

		bool IsValid() const { return Type != AssetType::None; }
	};
}
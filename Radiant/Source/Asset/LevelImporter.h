#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

#include "ECS/Level.h"

namespace Radiant {

	class LevelImporter
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static Ref<Level> ImportLevel(AssetHandle handle, const AssetMetadata& metadata);

		// Load from filepath
		static Ref<Level> LoadLevel(const std::filesystem::path& path);

		static void SaveLevel(Ref<Level> scene, const std::filesystem::path& path);
	};



}
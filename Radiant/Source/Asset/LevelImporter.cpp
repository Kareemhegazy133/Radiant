#include "rdpch.h"
#include "LevelImporter.h"

#include "ECS/LevelSerializer.h"

namespace Radiant {

	Ref<Level> LevelImporter::ImportLevel(AssetHandle handle, const AssetMetadata& metadata)
	{
		RADIANT_PROFILE_FUNCTION();

		return LoadLevel(metadata.FilePath);
	}

	Ref<Level> LevelImporter::LoadLevel(const std::filesystem::path& path)
	{
		RADIANT_PROFILE_FUNCTION();

		Ref<Level> level = CreateRef<Level>();
		LevelSerializer serializer(level);
		serializer.Deserialize(path);
		return level;
	}

	void LevelImporter::SaveLevel(Ref<Level> level, const std::filesystem::path& path)
	{
		LevelSerializer serializer(level);
		serializer.Serialize(path);
	}

}
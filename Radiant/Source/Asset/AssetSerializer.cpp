#include "rdpch.h"
#include "AssetSerializer.h"

#include "AssetManager.h"

#include "ECS/LevelSerializer.h"

namespace Radiant {

	//////////////////////////////////////////////////////////////////////////////////
	// LevelAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void LevelAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		const Ref<Level> level = std::static_pointer_cast<Level>(asset);
		LevelSerializer serializer(level);
		serializer.Serialize(AssetManager::GetFileSystemPath(metadata).string());
	}

	Ref<Asset> LevelAssetSerializer::Deserialize(const AssetMetadata& metadata) const
	{
		Ref<Level> level = CreateRef<Level>();
		LevelSerializer serializer(level);
		serializer.Deserialize(AssetManager::GetFileSystemPath(metadata).string());
		return level;
	}

	bool LevelAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Level> level = CreateRef<Level>("AssetPackTemp", false);
		const auto& metadata = AssetManager::GetMetadata(handle);
		LevelSerializer serializer(level);
		if (serializer.Deserialize(metadata.FilePath))
		{
			return serializer.SerializeToAssetPack(stream, outInfo);
		}
		return false;
	}

	Ref<Asset> LevelAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		RADIANT_ASSERT(false); // Not implemented
		return nullptr;
	}

	Ref<Level> LevelAssetSerializer::DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo) const
	{
		Ref<Level> level = CreateRef<Level>();
		LevelSerializer serializer(level);
		if (serializer.DeserializeFromAssetPack(stream, levelInfo))
			return level;

		return nullptr;
	}
}
#pragma once

#include "AssetMetadata.h"

#include "AssetSerializer.h"

#include "Serialization/FileStream.h"

#include "ECS/Level.h"

namespace Radiant {

	class AssetImporter
	{
	public:
		static void Init();

		static Ref<Asset> ImportAsset(const AssetMetadata& metadata);

		static bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
		static Ref<Level> DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo);

	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};

}
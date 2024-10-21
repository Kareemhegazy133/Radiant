#pragma once

#include "AssetMetadata.h"

#include "AssetSerializerAPI.h"

#include "Serialization/FileStream.h"

#include "ECS/Level.h"

namespace Radiant {

	class AssetSerializer
	{
	public:
		static void Init();

		static bool LoadAsset(const AssetMetadata& metadata, Ref<Asset>& asset);
		static void SaveAsset(const AssetMetadata& metadata, const Ref<Asset>& asset);

		static bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
		static Ref<Level> DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo);

	private:
		static std::unordered_map<AssetType, Scope<AssetSerializerAPI>> s_Serializers;
	};

}
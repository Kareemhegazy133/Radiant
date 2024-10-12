#include "rdpch.h"
#include "AssetSerializer.h"

#include "AssetManager.h"

namespace Radiant {

	std::unordered_map<AssetType, Scope<AssetSerializerAPI>> AssetSerializer::s_Serializers;

	void AssetSerializer::Init()
	{
		s_Serializers.clear();
		s_Serializers[AssetType::Level] = CreateScope<LevelAssetSerializerAPI>();
		//s_Serializers[AssetType::Texture2D] = CreateScope<TextureSerializerAPI>();
	}

	Ref<Asset> AssetSerializer::LoadAsset(const AssetMetadata& metadata)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			RADIANT_WARN("There's currently no serializer for assets of type {0}", Utils::AssetTypeToString(metadata.Type));
			return nullptr;
		}

		return s_Serializers[metadata.Type]->Deserialize(metadata);
	}

	void AssetSerializer::SaveAsset(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			RADIANT_WARN("There's currently no serializer for assets of type {0}", Utils::AssetTypeToString(metadata.Type));
			return;
		}

		return s_Serializers[metadata.Type]->Serialize(metadata, asset);
	}

	bool AssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		outInfo.Size = 0;

		if (!AssetManager::IsAssetHandleValid(handle))
			return false;

		AssetType type = AssetManager::GetAssetType(handle);
		if (s_Serializers.find(type) == s_Serializers.end())
		{
			RADIANT_WARN("There's currently no serializer for assets of type {0}", Utils::AssetTypeToString(type));
			return false;
		}

		return s_Serializers[type]->SerializeToAssetPack(handle, stream, outInfo);
	}

	Ref<Asset> AssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		AssetType assetType = static_cast<AssetType>(assetInfo.Type);
		if (s_Serializers.find(assetType) == s_Serializers.end())
			return nullptr;

		return s_Serializers[assetType]->DeserializeFromAssetPack(stream, assetInfo);
	}

	Ref<Level> AssetSerializer::DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo)
	{
		AssetType assetType = AssetType::Level;
		if (s_Serializers.find(assetType) == s_Serializers.end())
			return nullptr;

		LevelAssetSerializerAPI* levelAssetSerializerAPI = static_cast<LevelAssetSerializerAPI*>(s_Serializers[assetType].get());
		return levelAssetSerializerAPI->DeserializeLevelFromAssetPack(stream, levelInfo);
	}

}
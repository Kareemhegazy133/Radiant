#include "rdpch.h"
#include "AssetImporter.h"

#include "AssetManager.h"

namespace Radiant {

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

	void AssetImporter::Init()
	{
		s_Serializers.clear();
		s_Serializers[AssetType::Level] = CreateScope<LevelAssetSerializer>();
		//s_Serializers[AssetType::Texture2D] = CreateScope<TextureSerializer>();
	}

	Ref<Asset> AssetImporter::ImportAsset(const AssetMetadata& metadata)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			RADIANT_WARN("There's currently no importer for assets of type {0}", Utils::AssetTypeToString(metadata.Type));
			return nullptr;
		}

		return s_Serializers[metadata.Type]->Deserialize(metadata);
	}

	bool AssetImporter::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		outInfo.Size = 0;

		if (!AssetManager::IsAssetHandleValid(handle))
			return false;

		AssetType type = AssetManager::GetAssetType(handle);
		if (s_Serializers.find(type) == s_Serializers.end())
		{
			const auto& metadata = AssetManager::GetMetadata(handle);
			RADIANT_WARN("There's currently no serializer for assets of type {0}", Utils::AssetTypeToString(metadata.Type));
			return false;
		}

		return s_Serializers[type]->SerializeToAssetPack(handle, stream, outInfo);
	}

	Ref<Asset> AssetImporter::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		AssetType assetType = static_cast<AssetType>(assetInfo.Type);
		if (s_Serializers.find(assetType) == s_Serializers.end())
			return nullptr;

		return s_Serializers[assetType]->DeserializeFromAssetPack(stream, assetInfo);
	}

	Ref<Level> AssetImporter::DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo)
	{
		AssetType assetType = AssetType::Level;
		if (s_Serializers.find(assetType) == s_Serializers.end())
			return nullptr;

		LevelAssetSerializer* levelAssetSerializer = static_cast<LevelAssetSerializer*>(s_Serializers[assetType].get());
		return levelAssetSerializer->DeserializeLevelFromAssetPack(stream, levelInfo);
	}

}
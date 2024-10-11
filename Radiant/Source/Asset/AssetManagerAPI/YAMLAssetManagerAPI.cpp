#include "rdpch.h"
#include "YAMLAssetManagerAPI.h"

#include "Asset/AssetImporter.h"
#include "Asset/AssetExtensions.h"

namespace Radiant {

	Ref<Asset> YAMLAssetManagerAPI::LoadAsset(const std::filesystem::path& filepath)
	{
		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = filepath;
		metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
		RADIANT_ASSERT(metadata.Type != AssetType::None);

		Ref<Asset> asset = AssetImporter::ImportAsset(metadata);
		if (!asset)
		{
			RADIANT_ERROR("AssetManager: Failed to import asset at {0}", filepath.string());
		}

		asset->Handle = metadata.Handle;
		m_LoadedAssets[metadata.Handle] = asset;
		m_AssetRegistry.Set(metadata.Handle, metadata);
		SerializeAssetRegistry();
		return asset;
	}

	bool YAMLAssetManagerAPI::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return assetHandle != 0 && m_AssetRegistry.Contains(assetHandle);
	}

	bool YAMLAssetManagerAPI::IsAssetLoaded(AssetHandle assetHandle)
	{
		return m_LoadedAssets.find(assetHandle) != m_LoadedAssets.end();;
	}

	Ref<Asset> YAMLAssetManagerAPI::GetAsset(AssetHandle assetHandle)
	{
		RADIANT_PROFILE_FUNCTION();

		if (!IsAssetHandleValid(assetHandle))
			return nullptr;

		Ref<Asset> asset = nullptr;
		if (IsAssetLoaded(assetHandle))
		{
			asset = m_LoadedAssets.at(assetHandle);
		}
		else
		{
			const AssetMetadata& metadata = GetMetadata(assetHandle);

			asset = AssetImporter::ImportAsset(metadata);
			if (!asset)
			{
				RADIANT_ERROR("AssetManager: Failed to import asset");
			}
			m_LoadedAssets[assetHandle] = asset;
		}
		return asset;
	}

	AssetType YAMLAssetManagerAPI::GetAssetType(AssetHandle assetHandle)
	{
		Ref<Asset> asset = GetAsset(assetHandle);
		if (!asset)
			return AssetType::None;

		return asset->GetAssetType();
	}

	std::unordered_set<AssetHandle> YAMLAssetManagerAPI::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		for (const auto& [handle, metadata] : m_AssetRegistry)
		{
			if (metadata.Type == type)
				result.insert(handle);
		}

		return result;
	}

}
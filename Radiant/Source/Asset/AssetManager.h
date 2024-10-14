#pragma once

#include "Asset.h"
#include "AssetRegistry.h"
#include "Serialization/AssetPack.h"

namespace Radiant {

	class AssetManager
	{
	public:
		static void Init();

		static Ref<Asset> LoadAsset(const std::filesystem::path& filepath);
		static Ref<AssetPack> LoadAssetPack(const std::filesystem::path& filepath);

		static Ref<Level> LoadLevel(const std::filesystem::path& filepath);
		static void SaveLevel(const Ref<Level>& level, const std::filesystem::path& filepath);

		static bool IsAssetHandleValid(AssetHandle assetHandle);
		static bool IsAssetLoaded(AssetHandle assetHandle);

		static Ref<Asset> GetAsset(AssetHandle assetHandle);
		static AssetType GetAssetType(AssetHandle assetHandle);

		static std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

		static void SetMetadata(AssetHandle assetHandle, const AssetMetadata& metadata);
		static AssetMetadata GetMetadata(AssetHandle assetHandle);
		static const AssetMetadata& GetMetadata(const std::filesystem::path& filepath);

		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
		static std::filesystem::path GetFileSystemPath(AssetHandle assetHandle);
		static std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);

	private:
		static void SerializeAssetRegistry();
		static bool DeserializeAssetRegistry();

	private:
		struct AssetManagerData
		{
			AssetRegistry m_AssetRegistry;
			std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

			std::string AssetRegistryPath = "Assets/AssetRegistry.rdar";
		};
		
		static Scope<AssetManagerData> s_AssetManagerData;
	};

}
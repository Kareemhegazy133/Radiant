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

		template<typename T>
		static Ref<T> LoadAsset(const std::filesystem::path& filepath)
		{
			static_assert(std::is_base_of<Asset, T>::value, "LoadAsset<T> can only be used with types derived from Asset");

			// Call the original LoadAsset function
			Ref<Asset> baseAsset = LoadAsset(filepath);

			// Cast to the desired type T
			Ref<T> asset = std::static_pointer_cast<T>(baseAsset);
			if (!asset)
			{
				RADIANT_ERROR("AssetManager: Asset at {0} is not of type requested!", filepath.string());
				return nullptr;
			}

			return asset;
		}

		static Ref<AssetPack> LoadAssetPack(const std::filesystem::path& filepath);

		static void SaveLevel(const Ref<Level>& level, const std::filesystem::path& filepath);

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			// Ensure that T is derived from Asset
			static_assert(std::is_base_of<Asset, T>::value, "GetAsset<T> can only be used with types derived from Asset");

			// Call the original GetAsset function to retrieve the asset
			Ref<Asset> baseAsset = GetAsset(assetHandle);

			// Check if the asset exists
			if (!baseAsset)
			{
				RADIANT_ERROR("AssetManager: Asset with handle {0} not found!", (uint64_t)assetHandle);
				return nullptr;
			}

			// Attempt to cast to the requested type
			Ref<T> asset = std::static_pointer_cast<T>(baseAsset);
			if (!asset)
			{
				RADIANT_ERROR("AssetManager: Asset with handle {0} is not of the requested type!", (uint64_t)assetHandle);
				return nullptr;
			}

			return asset;
		}

		static Ref<Asset> GetAsset(AssetHandle assetHandle);

		static bool IsAssetHandleValid(AssetHandle assetHandle);
		static bool IsAssetLoaded(AssetHandle assetHandle);

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
#pragma once

#include "Core/Base.h"

#include "Asset.h"
#include "AssetRegistry.h"

namespace Radiant {

	class AssetManager
	{
	public:

		static void Init();
		//static void Shutdown();

		static Ref<Asset> ImportAsset(const std::filesystem::path& filepath);

		static bool IsAssetHandleValid(AssetHandle handle);
		static bool IsAssetLoaded(AssetHandle handle);
		static Ref<Asset> GetAsset(AssetHandle assetHandle);
		static AssetType GetAssetType(AssetHandle assetHandle);

		static std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

		static void SetMetadata(AssetHandle handle, const AssetMetadata& metadata);
		static AssetMetadata GetMetadata(AssetHandle handle);

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
		};

		static Scope<AssetManagerData> s_AssetManagerData;
	};
}
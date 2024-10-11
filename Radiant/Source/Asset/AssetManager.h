#pragma once

#include "AssetManagerAPI.h"

#include "Serialization/AssetPack.h"

namespace Radiant {

	class AssetManager
	{
	public:
		static void Init();

		static Ref<Asset> LoadAsset(const std::filesystem::path& filepath);
		static Ref<AssetPack> LoadAssetPack(const std::filesystem::path& filepath);

		static bool IsAssetHandleValid(AssetHandle handle);
		static bool IsAssetLoaded(AssetHandle handle);

		static Ref<Asset> GetAsset(AssetHandle assetHandle);
		static AssetType GetAssetType(AssetHandle assetHandle);

		static std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

		static void SetMetadata(AssetHandle assetHandle, const AssetMetadata& metadata);
		static AssetMetadata GetMetadata(AssetHandle assetHandle);
		static std::filesystem::path GetFileSystemPath(AssetHandle assetHandle);
		static std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);

	private:
		static Scope<AssetManagerAPI> s_AssetManagerAPI;
	};

}
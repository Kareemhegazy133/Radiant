#include "rdpch.h"
#include "BINARYAssetManagerAPI.h"

namespace Radiant {

	Ref<AssetPack> BINARYAssetManagerAPI::LoadAssetPack(const std::filesystem::path& filepath)
	{
		return Ref<AssetPack>();
	}

	bool BINARYAssetManagerAPI::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return false;
	}

	bool BINARYAssetManagerAPI::IsAssetLoaded(AssetHandle assetHandle)
	{
		return false;
	}

	Ref<Asset> BINARYAssetManagerAPI::GetAsset(AssetHandle assetHandle)
	{
		return Ref<Asset>();
	}

	AssetType BINARYAssetManagerAPI::GetAssetType(AssetHandle assetHandle)
	{
		return AssetType::None;
	}

	std::unordered_set<AssetHandle> BINARYAssetManagerAPI::GetAllAssetsWithType(AssetType type)
	{
		return std::unordered_set<AssetHandle>();
	}

}
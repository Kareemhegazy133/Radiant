#pragma once

#include "Asset/AssetManagerAPI.h"

namespace Radiant {

	class YAMLAssetManagerAPI : public AssetManagerAPI
	{
	public:

		virtual Ref<Asset> LoadAsset(const std::filesystem::path& filepath);

		virtual bool IsAssetHandleValid(AssetHandle assetHandle) override;
		virtual bool IsAssetLoaded(AssetHandle assetHandle) override;

		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		virtual AssetType GetAssetType(AssetHandle assetHandle) override;
		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) override;

	};

}
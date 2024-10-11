#pragma once

#include "Asset/AssetManagerAPI.h"
#include "Serialization/AssetPack.h"

namespace Radiant {

	class BINARYAssetManagerAPI : public AssetManagerAPI
	{
	public:

		virtual Ref<AssetPack> LoadAssetPack(const std::filesystem::path& filepath);

		virtual bool IsAssetHandleValid(AssetHandle assetHandle) override;
		virtual bool IsAssetLoaded(AssetHandle assetHandle) override;

		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		virtual AssetType GetAssetType(AssetHandle assetHandle) override;
		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) override;

	private:
		Ref<AssetPack> m_AssetPack;
	};

}
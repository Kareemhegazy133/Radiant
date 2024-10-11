#pragma once

#include "Asset/Asset.h"

#include "Asset/AssetRegistry.h"

namespace Radiant {

	class AssetManagerAPI
	{
	public:
		enum class API
		{
			YAML = 0, BINARY = 1
		};

	public:
		virtual ~AssetManagerAPI() = default;
		
		virtual bool IsAssetHandleValid(AssetHandle handle) = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) = 0;

		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) = 0;
		virtual AssetType GetAssetType(AssetHandle assetHandle) = 0;
		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) = 0;

		virtual void SetMetadata(AssetHandle handle, const AssetMetadata& metadata);
		virtual AssetMetadata GetMetadata(AssetHandle handle);

		virtual std::filesystem::path GetFileSystemPath(AssetHandle assetHandle);
		virtual std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);

		static API GetAPI() { return s_API; }
		static Scope<AssetManagerAPI> Create();

	protected:
		void SerializeAssetRegistry();
		bool DeserializeAssetRegistry();

	protected:
		AssetRegistry m_AssetRegistry;
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

	private:
		static API s_API;
	};

}
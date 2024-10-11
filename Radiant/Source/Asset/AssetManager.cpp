#include "rdpch.h"
#include "AssetManager.h"

#include "Asset/AssetImporter.h"

#include "AssetManagerAPI/YAMLAssetManagerAPI.h"
#include "AssetManagerAPI/BINARYAssetManagerAPI.h"

namespace Radiant {

	Scope<AssetManagerAPI> AssetManager::s_AssetManagerAPI = AssetManagerAPI::Create();

	void AssetManager::Init()
	{
		AssetImporter::Init();
	}

	Ref<Asset> AssetManager::LoadAsset(const std::filesystem::path& filepath)
	{
		RADIANT_ASSERT(s_AssetManagerAPI->GetAPI() == AssetManagerAPI::API::YAML, "AssetManager::LoadAsset() is a YAMLAssetManager Only function");
		
		YAMLAssetManagerAPI* yamlAM = static_cast<YAMLAssetManagerAPI*>(s_AssetManagerAPI.get());
		return yamlAM->LoadAsset(filepath);
	}

	Ref<AssetPack> AssetManager::LoadAssetPack(const std::filesystem::path& filepath)
	{
		RADIANT_ASSERT(s_AssetManagerAPI->GetAPI() == AssetManagerAPI::API::BINARY, "AssetManager::LoadAssetPack() is a BINARYAssetManager Only function");

		BINARYAssetManagerAPI* binaryAM = static_cast<BINARYAssetManagerAPI*>(s_AssetManagerAPI.get());
		return binaryAM->LoadAssetPack(filepath);
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle handle)
	{
		return s_AssetManagerAPI->IsAssetHandleValid(handle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return s_AssetManagerAPI->IsAssetLoaded(handle);
	}

	Ref<Asset> AssetManager::GetAsset(AssetHandle assetHandle)
	{
		return s_AssetManagerAPI->GetAsset(assetHandle);
	}

	AssetType AssetManager::GetAssetType(AssetHandle assetHandle)
	{
		return s_AssetManagerAPI->GetAssetType(assetHandle);
	}

	std::unordered_set<AssetHandle> AssetManager::GetAllAssetsWithType(AssetType type)
	{
		return s_AssetManagerAPI->GetAllAssetsWithType(type);
	}

	void AssetManager::SetMetadata(AssetHandle assetHandle, const AssetMetadata& metadata)
	{
		s_AssetManagerAPI->SetMetadata(assetHandle, metadata);
	}

	AssetMetadata AssetManager::GetMetadata(AssetHandle assetHandle)
	{
		return s_AssetManagerAPI->GetMetadata(assetHandle);
	}

	std::filesystem::path AssetManager::GetFileSystemPath(AssetHandle assetHandle)
	{
		return s_AssetManagerAPI->GetFileSystemPath(assetHandle);
	}

	std::filesystem::path AssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return s_AssetManagerAPI->GetFileSystemPath(metadata);
	}

}
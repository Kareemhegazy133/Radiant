#include "rdpch.h"
#include "AssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Asset/AssetSerializer.h"
#include "Asset/AssetTypes.h"
#include "Asset/AssetExtensions.h"

namespace Radiant {

	Scope<AssetManager::AssetManagerData> AssetManager::s_AssetManagerData = CreateScope<AssetManager::AssetManagerData>();

	static AssetMetadata s_NullMetadata;

	void AssetManager::Init()
	{
		AssetSerializer::Init();
	}

	Ref<Asset> AssetManager::LoadAsset(const std::filesystem::path& filepath)
	{
		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = filepath;
		metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
		RADIANT_ASSERT(metadata.Type != AssetType::None);

		Ref<Asset> asset = AssetSerializer::LoadAsset(metadata);
		if (!asset)
		{
			RADIANT_ERROR("AssetManager: Failed to import asset at {0}", filepath.string());
		}

		asset->Handle = metadata.Handle;
		s_AssetManagerData->m_LoadedAssets[metadata.Handle] = asset;
		s_AssetManagerData->m_AssetRegistry.Set(metadata.Handle, metadata);
		SerializeAssetRegistry();
		return asset;
	}

	Ref<Level> AssetManager::LoadLevel(const std::filesystem::path& filepath)
	{
		return static_pointer_cast<Level>(LoadAsset(filepath));
	}

	void AssetManager::SaveLevel(const Ref<Level>& level, const std::filesystem::path& filepath)
	{
		AssetMetadata metadata;
		metadata.Handle = level->Handle;
		metadata.FilePath = filepath;
		metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
		RADIANT_ASSERT(metadata.Type != AssetType::None);

		AssetSerializer::SaveAsset(metadata, level);
		s_AssetManagerData->m_AssetRegistry.Set(metadata.Handle, metadata);
		SerializeAssetRegistry();
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return assetHandle != 0 && s_AssetManagerData->m_AssetRegistry.Contains(assetHandle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle assetHandle)
	{
		return s_AssetManagerData->m_LoadedAssets.find(assetHandle) != s_AssetManagerData->m_LoadedAssets.end();;
	}

	Ref<Asset> AssetManager::GetAsset(AssetHandle assetHandle)
	{
		RADIANT_PROFILE_FUNCTION();

		if (!IsAssetHandleValid(assetHandle))
			return nullptr;

		Ref<Asset> asset = nullptr;
		if (IsAssetLoaded(assetHandle))
		{
			asset = s_AssetManagerData->m_LoadedAssets.at(assetHandle);
		}
		else
		{
			const AssetMetadata& metadata = GetMetadata(assetHandle);

			asset = AssetSerializer::LoadAsset(metadata);
			if (!asset)
			{
				RADIANT_ERROR("AssetManager: Failed to load asset");
			}
			s_AssetManagerData->m_LoadedAssets[assetHandle] = asset;
		}
		return asset;
	}

	AssetType AssetManager::GetAssetType(AssetHandle assetHandle)
	{
		Ref<Asset> asset = GetAsset(assetHandle);
		if (!asset)
			return AssetType::None;

		return asset->GetAssetType();
	}

	std::unordered_set<AssetHandle> AssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		for (const auto& [handle, metadata] : s_AssetManagerData->m_AssetRegistry)
		{
			if (metadata.Type == type)
				result.insert(handle);
		}

		return result;
	}

	void AssetManager::SetMetadata(AssetHandle assetHandle, const AssetMetadata& metadata)
	{
		s_AssetManagerData->m_AssetRegistry.Set(assetHandle, metadata);
	}

	AssetMetadata AssetManager::GetMetadata(AssetHandle assetHandle)
	{
		if (s_AssetManagerData->m_AssetRegistry.Contains(assetHandle))
			return s_AssetManagerData->m_AssetRegistry.Get(assetHandle);

		return s_NullMetadata;
	}

	std::filesystem::path AssetManager::GetFileSystemPath(AssetHandle assetHandle)
	{
		return GetFileSystemPath((GetMetadata(assetHandle)));
	}

	std::filesystem::path AssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return metadata.FilePath;
	}

	void AssetManager::SerializeAssetRegistry()
	{
		auto path = "Assets/AssetRegistry.rdar";
		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "AssetRegistry" << YAML::Value;

			out << YAML::BeginSeq;
			for (const auto& [handle, metadata] : s_AssetManagerData->m_AssetRegistry)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << handle;
				std::string filepathStr = metadata.FilePath.generic_string();
				out << YAML::Key << "FilePath" << YAML::Value << filepathStr;
				out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
			out << YAML::EndMap; // Root
		}

		std::ofstream fout(path);
		fout << out.c_str();
	}

	bool AssetManager::DeserializeAssetRegistry()
	{
		std::string path = "Assets/AssetRegistry.rdar";
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(path);
		}
		catch (YAML::ParserException e)
		{
			RADIANT_ERROR("Failed to load Asset Registry file: {0}, Error: {1}", path, e.what());
			return false;
		}

		auto rootNode = data["AssetRegistry"];
		if (!rootNode)
			return false;

		for (const auto& node : rootNode)
		{
			AssetHandle handle = node["Handle"].as<uint64_t>();
			auto& metadata = s_AssetManagerData->m_AssetRegistry.Get(handle);
			metadata.FilePath = node["FilePath"].as<std::string>();
			metadata.Type = Utils::AssetTypeFromString(node["Type"].as<std::string>());
		}

		return true;
	}

}
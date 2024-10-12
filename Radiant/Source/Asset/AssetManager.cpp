#include "rdpch.h"
#include "AssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Asset/AssetSerializer.h"
#include "Asset/AssetTypes.h"
#include "Asset/AssetExtensions.h"

#include "Utilities/FileSystem.h"

namespace Radiant {

	Scope<AssetManager::AssetManagerData> AssetManager::s_AssetManagerData = CreateScope<AssetManager::AssetManagerData>();

	static AssetMetadata s_NullMetadata;

	void AssetManager::Init()
	{
		AssetSerializer::Init();

		if (!DeserializeAssetRegistry())
			RADIANT_WARN("AssetManager: Failed to Deserialize Asset Registry");
	}

	Ref<Asset> AssetManager::LoadAsset(const std::filesystem::path& filepath)
	{
		AssetMetadata metadata = GetMetadata(filepath);

		if (!IsAssetHandleValid(metadata.Handle))
		{
			metadata.Handle = AssetHandle();
			metadata.FilePath = filepath;
			metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
			RADIANT_ASSERT(metadata.Type != AssetType::None);
		}

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

	const AssetMetadata& AssetManager::GetMetadata(const std::filesystem::path& filepath)
	{
		for (auto& [handle, metadata] : s_AssetManagerData->m_AssetRegistry)
		{
			if (metadata.FilePath == filepath)
				return metadata;
		}

		return s_NullMetadata;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath)
	{
		return GetMetadata(filepath).Handle;
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromFileExtension(path.extension());
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
		const std::string& assetRegistryPath = "Assets/AssetRegistry.rdar";
		if (!FileSystem::Exists(assetRegistryPath))
		{
			RADIANT_WARN("Asset Manager: AssetRegistry file at {0} was not found", assetRegistryPath);
			return false;
		}

		std::ifstream stream(assetRegistryPath);
		RADIANT_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		auto rootNode = data["AssetRegistry"];
		if (!rootNode)
		{
			RADIANT_ERROR("AssetManager: Asset Registry file appears to be corrupted!");
			return false;
		}

		for (const auto& node : rootNode)
		{
			std::string filepath = node["FilePath"].as<std::string>();

			AssetMetadata metadata;
			metadata.Handle = node["Handle"].as<uint64_t>();
			metadata.FilePath = filepath;
			metadata.Type = Utils::AssetTypeFromString(node["Type"].as<std::string>());

			if (metadata.Type == AssetType::None)
				continue;

			if (metadata.Type != GetAssetTypeFromPath(filepath))
			{
				RADIANT_WARN("AssetManager: Mismatch between stored AssetType and extension type when reading asset registry!");
				metadata.Type = GetAssetTypeFromPath(filepath);
			}

			if (metadata.Handle == 0)
			{
				RADIANT_WARN("AssetManager: AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath.string());
				continue;
			}

			SetMetadata(metadata.Handle, metadata);
		}

		RADIANT_INFO("AssetManager: Loaded {0} asset entries", s_AssetManagerData->m_AssetRegistry.Count());

		return true;
	}

}
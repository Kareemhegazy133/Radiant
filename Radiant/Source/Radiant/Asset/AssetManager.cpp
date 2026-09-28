#include "Radiant/rdpch.h"
#include "AssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Radiant/Asset/AssetSerializer.h"
#include "Radiant/Asset/AssetTypes.h"
#include "Radiant/Asset/AssetExtensions.h"

#include "Radiant/Utilities/FileSystem.h"

namespace Radiant {

	Scope<AssetManager::AssetManagerData> AssetManager::s_AssetManagerData = CreateScope<AssetManager::AssetManagerData>();

	// Shared "not found" record (IsValid() == false) — also backs the by-reference
	// GetMetadata overload, so it must have static storage duration and never be mutated.
	static AssetMetadata s_NullMetadata;

	void AssetManager::Init()
	{
		AssetSerializer::Init();
	}

	Ref<Asset> AssetManager::LoadAsset(const std::filesystem::path& filepath)
	{
		AssetMetadata metadata = GetMetadata(filepath);

		if (!IsAssetHandleValid(metadata.Handle))
		{
			// Unknown path: mint a fresh handle and register on the fly. This is an
			// import operation fused into a runtime API — known design debt (RAD-43);
			// the same unregistered file gets a different handle every run.
			metadata.Handle = AssetHandle();
			metadata.FilePath = filepath;
			metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
			// Unsupported extension is a content mistake (the extension helper
			// already warned) — recover with null, don't assert
			if (metadata.Type == AssetType::None)
			{
				RADIANT_WARN("AssetManager: cannot import '{0}' - unsupported asset type", filepath.string());
				return nullptr;
			}
		}

		if (IsAssetLoaded(metadata.Handle))
		{
			// Normal cache hit on the import path — not a warning
			RADIANT_TRACE("AssetManager: asset {0} already loaded, returning cached", (uint64_t)metadata.Handle);
			return s_AssetManagerData->m_LoadedAssets.at(metadata.Handle);
		}

		Ref<Asset> asset = nullptr;
		bool result = AssetSerializer::LoadAsset(metadata, asset);
		if (!result || !asset)
		{
			// Never register or cache a failed load — a poisoned cache makes retries impossible
			RADIANT_ERROR("AssetManager: Failed to import asset at {0}", filepath.string());
			return nullptr;
		}

		asset->Handle = metadata.Handle;
		s_AssetManagerData->m_LoadedAssets[metadata.Handle] = asset;
		s_AssetManagerData->m_AssetRegistry.Set(metadata.Handle, metadata);

		return asset;
	}

	void AssetManager::SaveLevel(const Ref<Level>& level, const std::filesystem::path& filepath)
	{
		AssetMetadata metadata;
		metadata.Handle = level->Handle;
		metadata.FilePath = filepath;
		metadata.Type = GetAssetTypeFromFileExtension(filepath.extension());
		if (metadata.Type == AssetType::None)
		{
			RADIANT_WARN("AssetManager: cannot save level to '{0}' - unsupported extension", filepath.string());
			return;
		}

		AssetSerializer::SaveAsset(metadata, level);
		s_AssetManagerData->m_AssetRegistry.Set(metadata.Handle, metadata);
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

			bool result = AssetSerializer::LoadAsset(metadata, asset);
			if (!result || !asset)
			{
				// Never cache a failed load — a cached null poisons every retry
				RADIANT_ERROR("AssetManager: Failed to load asset with handle {0} from {1}",
					(uint64_t)assetHandle, metadata.FilePath.string());
				return nullptr;
			}
			s_AssetManagerData->m_LoadedAssets[assetHandle] = asset;
		}
		return asset;
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return assetHandle != 0 && s_AssetManagerData->m_AssetRegistry.Contains(assetHandle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle assetHandle)
	{
		return s_AssetManagerData->m_LoadedAssets.find(assetHandle) != s_AssetManagerData->m_LoadedAssets.end();;
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
		// Linear scan is acceptable here: path lookups happen at import/registry
		// time only, never per-frame — runtime resolution is by handle.
		for (auto& [handle, metadata] : s_AssetManagerData->m_AssetRegistry)
		{
			if (metadata.FilePath == filepath)
				return metadata;
		}

		return s_NullMetadata;
	}

	Ref<Asset> AssetManager::GetAssetFromFilePath(const std::filesystem::path& filepath)
	{
		return GetAsset(GetAssetHandleFromFilePath(filepath));
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

	bool AssetManager::LoadAssetRegistry(const std::filesystem::path& assetRegistryPath)
	{
		if (!FileSystem::Exists(assetRegistryPath))
		{
			RADIANT_WARN("AssetManager: AssetRegistry file at {0} was not found", assetRegistryPath.string());
			return false;
		}

		// An unreadable/locked registry is a content/config failure — the function
		// contract is "return false when malformed", so recover, don't assert
		std::ifstream stream(assetRegistryPath);
		if (!stream)
		{
			RADIANT_WARN("AssetManager: AssetRegistry at {0} could not be opened", assetRegistryPath.string());
			return false;
		}
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data;
		try
		{
			data = YAML::Load(strStream.str());
		}
		catch (const YAML::Exception& ex)
		{
			RADIANT_ERROR("AssetManager: failed to parse AssetRegistry {0}: {1}", assetRegistryPath.string(), ex.what());
			return false;
		}

		auto rootNode = data["AssetRegistry"];
		if (!rootNode)
		{
			RADIANT_ERROR("AssetManager: Asset Registry file appears to be corrupted!");
			return false;
		}

		for (const auto& node : rootNode)
		{
			AssetMetadata metadata;
			try
			{
				metadata.Handle = node["Handle"].as<uint64_t>();
				metadata.FilePath = node["FilePath"].as<std::string>();
				metadata.Type = Utils::AssetTypeFromString(node["Type"].as<std::string>());
			}
			catch (const YAML::Exception& ex)
			{
				// One malformed entry must not discard the rest of the registry
				RADIANT_WARN("AssetManager: skipping malformed AssetRegistry entry: {0}", ex.what());
				continue;
			}

			if (metadata.Type == AssetType::None)
				continue;

			if (metadata.Type != GetAssetTypeFromPath(metadata.FilePath))
			{
				RADIANT_WARN("AssetManager: Mismatch between stored AssetType and extension type when reading asset registry!");
				metadata.Type = GetAssetTypeFromPath(metadata.FilePath);
			}

			if (metadata.Handle == 0)
			{
				RADIANT_WARN("AssetManager: AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath.string());
				continue;
			}

			SetMetadata(metadata.Handle, metadata);
		}

		RADIANT_INFO("AssetManager: Loaded AssetRegistry file at {0} with {1} asset entries", assetRegistryPath.string(), s_AssetManagerData->m_AssetRegistry.Count());

		return true;
	}

	void AssetManager::SaveAssetRegistry(const std::filesystem::path& assetRegistryPath)
	{
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

		std::ofstream fout(assetRegistryPath);
		if (!fout)
		{
			RADIANT_WARN("AssetManager: failed to open {0} for writing - registry not saved", assetRegistryPath.string());
			return;
		}
		fout << out.c_str();

		RADIANT_INFO("AssetManager: AssetRegistry file saved at {0}", assetRegistryPath.string());
	}

	void AssetManager::ClearAssets()
	{
		s_AssetManagerData->m_LoadedAssets.clear();
		RADIANT_INFO("AssetManager: Cleared Loaded Assets");
		s_AssetManagerData->m_AssetRegistry.Clear();
		RADIANT_INFO("AssetManager: Cleared AssetRegistry");
	}

}
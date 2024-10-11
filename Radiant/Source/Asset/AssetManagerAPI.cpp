#include "rdpch.h"
#include "AssetManagerAPI.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "Asset/AssetTypes.h"

#include "AssetManagerAPI/YAMLAssetManagerAPI.h"
#include "AssetManagerAPI/BINARYAssetManagerAPI.h"

namespace Radiant {

	static AssetMetadata s_NullMetadata;

	AssetManagerAPI::API AssetManagerAPI::s_API = AssetManagerAPI::API::YAML;

	Scope<AssetManagerAPI> AssetManagerAPI::Create()
	{
		switch (s_API)
		{
		case AssetManagerAPI::API::YAML:
			return CreateScope<YAMLAssetManagerAPI>();
		case AssetManagerAPI::API::BINARY:
			return CreateScope<BINARYAssetManagerAPI>();
		}

		RADIANT_ASSERT(false, "Unknown AssetManagerAPI!");
		return nullptr;
	}


	void AssetManagerAPI::SetMetadata(AssetHandle assetHandle, const AssetMetadata& metadata)
	{
		m_AssetRegistry.Set(assetHandle, metadata);
	}

	AssetMetadata AssetManagerAPI::GetMetadata(AssetHandle assetHandle)
	{
		if (m_AssetRegistry.Contains(assetHandle))
			return m_AssetRegistry.Get(assetHandle);

		return s_NullMetadata;
	}

	std::filesystem::path AssetManagerAPI::GetFileSystemPath(AssetHandle assetHandle)
	{
		return GetFileSystemPath((GetMetadata(assetHandle)));
	}

	std::filesystem::path AssetManagerAPI::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return metadata.FilePath;
	}

	void AssetManagerAPI::SerializeAssetRegistry()
	{
		auto path = "Assets/AssetRegistry.rdar";
		YAML::Emitter out;
		{
			out << YAML::BeginMap; // Root
			out << YAML::Key << "AssetRegistry" << YAML::Value;

			out << YAML::BeginSeq;
			for (const auto& [handle, metadata] : m_AssetRegistry)
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

	bool AssetManagerAPI::DeserializeAssetRegistry()
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
			auto& metadata = m_AssetRegistry.Get(handle);
			metadata.FilePath = node["FilePath"].as<std::string>();
			metadata.Type = Utils::AssetTypeFromString(node["Type"].as<std::string>());
		}

		return true;
	}

}
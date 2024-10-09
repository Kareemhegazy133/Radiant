#include "rdpch.h"
#include "AssetPack.h"

#include "Asset/AssetManager.h"
#include "Asset/AssetImporter.h"
#include "Core/Platform.h"
#include "ECS/Level.h"
#include "ECS/LevelSerializer.h"

namespace Radiant {

	AssetPack::AssetPack(const std::filesystem::path& path)
		: m_Path(path)
	{
	}

	Ref<Level> AssetPack::LoadLevel(AssetHandle levelHandle)
	{
		auto it = m_File.Index.Levels.find(levelHandle);
		if (it == m_File.Index.Levels.end())
			return nullptr;

		const AssetPackFile::LevelInfo& levelInfo = it->second;

		FileStreamReader stream(m_Path);
		Ref<Level> level = AssetImporter::DeserializeLevelFromAssetPack(stream, levelInfo);
		level->Handle = levelHandle;
		return level;
	}

	Ref<Asset> AssetPack::LoadAsset(AssetHandle levelHandle, AssetHandle assetHandle)
	{
		const AssetPackFile::AssetInfo* assetInfo = nullptr;

		bool foundAsset = false;
		if (levelHandle)
		{
			// Fast(er) path
			auto it = m_File.Index.Levels.find(levelHandle);
			if (it != m_File.Index.Levels.end())
			{
				const AssetPackFile::LevelInfo& levelInfo = it->second;
				auto assetIt = levelInfo.Assets.find(assetHandle);
				if (assetIt != levelInfo.Assets.end())
				{
					foundAsset = true;
					assetInfo = &assetIt->second;
				}
			}
		}

		if (!foundAsset)
		{
			// Slow(er) path
			for (const auto& [handle, levelInfo] : m_File.Index.Levels)
			{
				auto assetIt = levelInfo.Assets.find(assetHandle);
				if (assetIt != levelInfo.Assets.end())
				{
					assetInfo = &assetIt->second;
					break;
				}
			}

			if (!assetInfo)
				return nullptr;
		}

		FileStreamReader stream(m_Path);
		Ref<Asset> asset = AssetImporter::DeserializeFromAssetPack(stream, *assetInfo);
		//RADIANT_ASSERT(asset);
		if (!asset)
			return nullptr;

		asset->Handle = assetHandle;
		return asset;
	}

	bool AssetPack::IsAssetHandleValid(AssetHandle assetHandle) const
	{
		return m_AssetHandleIndex.find(assetHandle) != m_AssetHandleIndex.end();
	}

	bool AssetPack::IsAssetHandleValid(AssetHandle levelHandle, AssetHandle assetHandle) const
	{
		auto levelIterator = m_File.Index.Levels.find(levelHandle);
		if (levelIterator == m_File.Index.Levels.end())
			return false;

		const auto& levelInfo = levelIterator->second;
		return levelInfo.Assets.find(assetHandle) != levelInfo.Assets.end();
	}

	Buffer AssetPack::ReadAppBinary()
	{
		FileStreamReader stream(m_Path);
		stream.SetStreamPosition(m_File.Index.PackedAppBinaryOffset);
		Buffer buffer;
		stream.ReadBuffer(buffer);
		RADIANT_ASSERT(m_File.Index.PackedAppBinarySize == (buffer.Size + sizeof(uint64_t)));
		return buffer;
	}

	uint64_t AssetPack::GetBuildVersion()
	{
		return m_File.Header.BuildVersion;
	}

	AssetType AssetPack::GetAssetType(AssetHandle levelHandle, AssetHandle assetHandle) const
	{
		const AssetPackFile::AssetInfo* assetInfo = nullptr;

		bool foundAsset = false;
		if (levelHandle)
		{
			// Fast(er) path
			auto it = m_File.Index.Levels.find(levelHandle);
			if (it != m_File.Index.Levels.end())
			{
				const AssetPackFile::LevelInfo& levelInfo = it->second;
				auto assetIt = levelInfo.Assets.find(assetHandle);
				if (assetIt != levelInfo.Assets.end())
				{
					foundAsset = true;
					assetInfo = &assetIt->second;
				}
			}
		}

		if (!foundAsset)
		{
			// Slow(er) path
			for (const auto& [handle, levelInfo] : m_File.Index.Levels)
			{
				auto assetIt = levelInfo.Assets.find(assetHandle);
				if (assetIt != levelInfo.Assets.end())
				{
					assetInfo = &assetIt->second;
					break;
				}
			}

			if (!assetInfo)
				return AssetType::None;
		}

		return (AssetType)assetInfo->Type;
	}

	Ref<AssetPack> AssetPack::Create(const std::filesystem::path& path)
	{
#define DEBUG_PRINT 1

		AssetPackFile assetPackFile;
		assetPackFile.Header.BuildVersion = Platform::GetCurrentDateTimeU64();

		std::unordered_set<AssetHandle> fullAssetList;

		std::unordered_set<AssetHandle> levelHandles = AssetManager::GetAllAssetsWithType(AssetType::Level);
		uint32_t levelCount = (uint32_t)levelHandles.size();

		if (levelCount == 0)
		{
			RADIANT_ERROR("There are no levels in the game. Nothing to serialize to asset pack!");
			return nullptr;
		}

		for (const auto levelHandle : levelHandles)
		{
			const auto metadata = AssetManager::GetMetadata(levelHandle);

			Ref<Level> level = CreateRef<Level>("AssetPack", false);
			LevelSerializer serializer(level);
			RADIANT_TRACE("AssetPack: Deserializing Level: {0}", metadata.FilePath.string());
			if (serializer.Deserialize(metadata.FilePath))
			{
				std::unordered_set<AssetHandle> levelAssetList = level->GetAssetList();
				RADIANT_TRACE("AssetPack: Level has {0} used assets", levelAssetList.size());

				AssetPackFile::LevelInfo& levelInfo = assetPackFile.Index.Levels[levelHandle];
				for (AssetHandle assetHandle : levelAssetList)
				{
					AssetPackFile::AssetInfo& assetInfo = levelInfo.Assets[assetHandle];
					assetInfo.Type = (uint16_t)AssetManager::GetAssetType(assetHandle);
				}

				fullAssetList.insert(levelAssetList.begin(), levelAssetList.end());
			}
			else
			{
				RADIANT_ERROR("AssetPack: Failed to deserialize Level: {0} ({1})", metadata.FilePath.string(), (uint64_t)levelHandle);
			}
		}

		RADIANT_INFO("Level contains {0} used assets", fullAssetList.size());

		Buffer appBinary;

		AssetPackSerializer::Serialize(path, assetPackFile, appBinary);

		std::unordered_map<AssetHandle, AssetPackFile::AssetInfo> serializedAssets;
		for (auto& [levelHandle, levelInfo] : assetPackFile.Index.Levels)
		{
			for (auto& [assetHandle, assetInfo] : levelInfo.Assets)
			{
				if (serializedAssets.find(assetHandle) == serializedAssets.end())
				{
					serializedAssets[assetHandle] = assetInfo;
				}
			}
		}

		RADIANT_INFO("Asset Pack", "Serialized Assets:");
		for (const auto& [handle, info] : serializedAssets)
		{
			const auto& metadata = AssetManager::GetMetadata(handle);
			RADIANT_INFO("Asset Pack", "{0}: {1} (offset = {2}, size = {3})", Utils::AssetTypeToString(metadata.Type), metadata.FilePath.string(), info.PackedOffset, info.PackedSize);
		}

		return nullptr;
	}

	Ref<AssetPack> AssetPack::Load(const std::filesystem::path& path)
	{
		Ref<AssetPack> assetPack = CreateRef<AssetPack>();
		assetPack->m_Path = path;
		bool success = AssetPackSerializer::DeserializeIndex(assetPack->m_Path, assetPack->m_File);
		RADIANT_ASSERT(success);
		if (!success)
			return nullptr;

		// Populate asset handle index
		const auto& index = assetPack->m_File.Index;
		for (const auto& [levelHandle, levelInfo] : index.Levels)
		{
			assetPack->m_AssetHandleIndex.insert(levelHandle);
			for (const auto& [assetHandle, assetInfo] : levelInfo.Assets)
			{
				assetPack->m_AssetHandleIndex.insert(assetHandle);
			}
		}

		// Debug log
#ifndef RD_DIST
		{
			RADIANT_INFO("Asset Pack", "-----------------------------------------------------");
			RADIANT_INFO("Asset Pack", "AssetPack Dump {0}", assetPack->m_Path.string());
			RADIANT_INFO("Asset Pack", "-----------------------------------------------------");
			std::unordered_map<AssetType, uint32_t> typeCounts;
			std::unordered_set<AssetHandle> duplicatePreventionSet;
			for (const auto& [levelHandle, levelInfo] : index.Levels)
			{
				RADIANT_INFO("Asset Pack", "Level {0}:", levelHandle);
				for (const auto& [assetHandle, assetInfo] : levelInfo.Assets)
				{
					RADIANT_INFO("Asset Pack", "  {0} - {1}", Utils::AssetTypeToString((AssetType)assetInfo.Type), assetHandle);

					if (duplicatePreventionSet.find(assetHandle) == duplicatePreventionSet.end())
					{
						duplicatePreventionSet.insert(assetHandle);
						typeCounts[(AssetType)assetInfo.Type]++;
					}
				}
			}
			RADIANT_INFO("Asset Pack", "-----------------------------------------------------");
			RADIANT_INFO("Asset Pack", "Summary:");
			for (const auto& [type, count] : typeCounts)
			{
				RADIANT_INFO("Asset Pack", "  {0} {1}", count, Utils::AssetTypeToString(type));
			}
			RADIANT_INFO("Asset Pack", "-----------------------------------------------------");
		}
#endif

		return assetPack;
	}

}
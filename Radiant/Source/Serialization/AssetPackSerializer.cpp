#include "rdpch.h"
#include "AssetPackSerializer.h"

#include <filesystem>
#include <fstream>

#include "Asset/AssetImporter.h"

#include "Serialization/FileStream.h"

namespace Radiant {

	static void CreateDirectoriesIfNeeded(const std::filesystem::path& path)
	{
		std::filesystem::path directory = path.parent_path();
		if (!std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);
	}

	void AssetPackSerializer::Serialize(const std::filesystem::path& path, AssetPackFile& file, Buffer appBinary)
	{
		// Print Info
		RADIANT_INFO("Serializing AssetPack to {}", path.string());
		RADIANT_INFO("  {} levels", file.Index.Levels.size());
		uint32_t assetCount = 0;
		for (const auto& [levelHandle, levelInfo] : file.Index.Levels)
			assetCount += uint32_t(levelInfo.Assets.size());
		RADIANT_INFO("  {} assets (including duplicates)", assetCount);

		FileStreamWriter serializer(path);

		serializer.WriteRaw<AssetPackFile::FileHeader>(file.Header);

		// ===============
		// Write index
		// ===============
		// Write dummy data for index (come back later to fill in)
		uint64_t indexPos = serializer.GetStreamPosition();
		uint64_t indexTableSize = CalculateIndexTableSize(file);
		serializer.WriteZero(indexTableSize);

		std::unordered_map<AssetHandle, AssetSerializationInfo> serializedAssets;

		// Write app binary data
		file.Index.PackedAppBinaryOffset = serializer.GetStreamPosition();
		serializer.WriteBuffer(appBinary);
		file.Index.PackedAppBinarySize = serializer.GetStreamPosition() - file.Index.PackedAppBinaryOffset;
		appBinary.Release();

		// Write asset data + fill in offset + size
		for (auto& [levelHandle, levelInfo] : file.Index.Levels)
		{
			// Serialize Level
			AssetSerializationInfo serializationInfo;
			AssetImporter::SerializeToAssetPack(levelHandle, serializer, serializationInfo);
			file.Index.Levels[levelHandle].PackedOffset = serializationInfo.Offset;
			file.Index.Levels[levelHandle].PackedSize = serializationInfo.Size;

			// Serialize Assets
			for (auto& [assetHandle, assetInfo] : levelInfo.Assets)
			{
				if (serializedAssets.find(assetHandle) != serializedAssets.end())
				{
					// Has already been serialized
					serializationInfo = serializedAssets.at(assetHandle);
					file.Index.Levels[levelHandle].Assets[assetHandle].PackedOffset = serializationInfo.Offset;
					file.Index.Levels[levelHandle].Assets[assetHandle].PackedSize = serializationInfo.Size;
				}
				else
				{
					// Serialize asset
					if (AssetImporter::SerializeToAssetPack(assetHandle, serializer, serializationInfo))
					{
						file.Index.Levels[levelHandle].Assets[assetHandle].PackedOffset = serializationInfo.Offset;
						file.Index.Levels[levelHandle].Assets[assetHandle].PackedSize = serializationInfo.Size;
						serializedAssets[assetHandle] = serializationInfo;
					}
					else
					{
						RADIANT_ERROR("Failed to serialize asset with handle {}", assetHandle);
					}
				}
			}
		}

		RADIANT_INFO("Serialized {} assets into AssetPack", serializedAssets.size());

		serializer.SetStreamPosition(indexPos);
		serializer.WriteRaw<uint64_t>(file.Index.PackedAppBinaryOffset);
		serializer.WriteRaw<uint64_t>(file.Index.PackedAppBinarySize);

		uint64_t begin = indexPos;
		serializer.WriteRaw<uint32_t>((uint32_t)file.Index.Levels.size()); // Write level map size
		for (auto& [levelHandle, levelInfo] : file.Index.Levels)
		{
			serializer.WriteRaw<uint64_t>(levelHandle);
			serializer.WriteRaw<uint64_t>(levelInfo.PackedOffset);
			serializer.WriteRaw<uint64_t>(levelInfo.PackedSize);
			serializer.WriteRaw<uint16_t>(levelInfo.Flags);

			serializer.WriteMap(file.Index.Levels[levelHandle].Assets);
		}
	}

	bool AssetPackSerializer::DeserializeIndex(const std::filesystem::path& path, AssetPackFile& file)
	{
		// Print Info
		RADIANT_INFO("Asset Pack", "Deserializing Asset Pack from {0}", path.string());

		FileStreamReader stream(path);
		if (!stream.IsStreamGood())
			return false;

		stream.ReadRaw<AssetPackFile::FileHeader>(file.Header);
		bool validHeader = memcmp(file.Header.HEADER, "RDAP", 4) == 0;
		RADIANT_ASSERT(validHeader);
		if (!validHeader)
			return false;

		AssetPackFile current;
		if (file.Header.Version != current.Header.Version)
		{
			RADIANT_ERROR("AssetPack version {0} is not compatible with current version {1}", file.Header.Version, current.Header.Version);
			return false;
		}

		// Read app binary info
		stream.ReadRaw<uint64_t>(file.Index.PackedAppBinaryOffset);
		stream.ReadRaw<uint64_t>(file.Index.PackedAppBinarySize);

		uint32_t levelCount = 0;
		stream.ReadRaw<uint32_t>(levelCount); // Read level map size
		for (uint32_t i = 0; i < levelCount; i++)
		{
			uint64_t levelHandle = 0;
			stream.ReadRaw<uint64_t>(levelHandle);

			AssetPackFile::LevelInfo& levelInfo = file.Index.Levels[levelHandle];
			stream.ReadRaw<uint64_t>(levelInfo.PackedOffset);
			stream.ReadRaw<uint64_t>(levelInfo.PackedSize);
			stream.ReadRaw<uint16_t>(levelInfo.Flags);

			stream.ReadMap(levelInfo.Assets);
		}

		RADIANT_INFO("Asset Pack", "Deserialized index with {0} levels from AssetPack", levelCount);
		return true;
	}

	uint64_t AssetPackSerializer::CalculateIndexTableSize(const AssetPackFile& file)
	{
		uint64_t appInfoSize = sizeof(uint64_t) * 2;
		uint64_t levelMapSize = sizeof(uint32_t) + (sizeof(AssetHandle) + sizeof(uint64_t) * 2 + sizeof(uint16_t)) * file.Index.Levels.size();
		uint64_t assetMapSize = 0;
		for (const auto& [levelHandle, levelInfo] : file.Index.Levels)
			assetMapSize += sizeof(uint32_t) + (sizeof(AssetHandle) + sizeof(AssetPackFile::AssetInfo)) * levelInfo.Assets.size();

		return appInfoSize + levelMapSize + assetMapSize;
	}

}
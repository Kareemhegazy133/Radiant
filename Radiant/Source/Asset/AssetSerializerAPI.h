#pragma once

#include "AssetMetadata.h"

#include "Serialization/FileStream.h"
#include "Serialization/AssetPackFile.h"

namespace Radiant {

	class Level;

	struct AssetSerializationInfo
	{
		uint64_t Offset = 0;
		uint64_t Size = 0;
	};

	class AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual Ref<Asset> Deserialize(const AssetMetadata& metadata) const = 0;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const = 0;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const = 0;
	};

	class TextureSerializerAPI : public AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual Ref<Asset> Deserialize(const AssetMetadata& metadata) const override;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const;
	};

	class LevelAssetSerializerAPI : public AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual Ref<Asset> Deserialize(const AssetMetadata& metadata) const override;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override;
		Ref<Level> DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo) const;
	};

}
#pragma once

#include "AssetMetadata.h"

#include "Radiant/Serialization/FileStream.h"
#include "Radiant/Serialization/AssetPackFile.h"

namespace Radiant {

	class Level;

	/** Where a payload landed in a pack stream (byte offset + size), reported by SerializeToAssetPack for the pack index. Pack path is parked (RAD-46). */
	struct AssetSerializationInfo
	{
		uint64_t Offset = 0;
		uint64_t Size = 0;
	};

	/**
	 * Per-asset-type serialization plug-in: one stateless implementation per
	 * AssetType, owned (as Scope) by AssetSerializer's registry and registered in
	 * AssetSerializer::Init. Serialize/Deserialize work on loose source files
	 * located via metadata.FilePath; Deserialize returns false on failure so the
	 * AssetManager can uphold its nullptr-and-never-cache contract (RAD-13). The
	 * *AssetPack methods belong to the parked Phase 4 pack path (RAD-46).
	 */
	class AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual bool Deserialize(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const = 0;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const = 0;
	};

	/**
	 * Decodes image files (.png/.jpg/.jpeg) via stb_image into a Texture2D,
	 * forcing RGBA. Load-only: textures are source content and are never written
	 * back, so Serialize asserts.
	 */
	class TextureSerializerAPI : public AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool Deserialize(const AssetMetadata& metadata, Ref<Asset>& asset) const override;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const;
	};

	/** Loads and saves levels as YAML .rdlvl files by delegating to LevelSerializer. */
	class LevelAssetSerializerAPI : public AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool Deserialize(const AssetMetadata& metadata, Ref<Asset>& asset) const override;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override;
		Ref<Level> DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo) const;
	};

	/**
	 * Declared but not implemented or registered anywhere — parked until the MSDF
	 * font pipeline revives and Font becomes a managed asset type (RAD-47).
	 * Instantiating it fails at link time.
	 */
	class FontSerializerAPI : public AssetSerializerAPI
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool Deserialize(const AssetMetadata& metadata, Ref<Asset>& asset) const override;

		virtual bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const;
	};

}
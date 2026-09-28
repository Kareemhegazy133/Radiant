#include "Radiant/rdpch.h"
#include "AssetSerializerAPI.h"

#include <stb_image.h>

#include "AssetManager.h"

#include "Radiant/ECS/LevelSerializer.h"

#include "Radiant/Utilities/FileSystem.h"

namespace Radiant {

	//////////////////////////////////////////////////////////////////////////////////
	// TextureSerializerAPI
	//////////////////////////////////////////////////////////////////////////////////

	void TextureSerializerAPI::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		RADIANT_ASSERT(false, "Serialization path not supported for this asset type"); // Not needed
	}

	bool TextureSerializerAPI::Deserialize(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		RADIANT_PROFILE_FUNCTION();

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		Buffer data;

		{
			RADIANT_PROFILE_SCOPE("stbi_load - TextureSerializerAPI::Deserialize");
			std::string pathStr = metadata.FilePath.string();
			data.Data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
			// stbi reports the file's ORIGINAL channel count; req_comp=4 expanded the
			// data to RGBA, so size math and format selection must use 4.
			channels = 4;
		}

		if (data.Data == nullptr)
		{
			RADIANT_ERROR("TextureSerializerAPI::Deserialize - Could not load texture from filepath: {0}", metadata.FilePath.string());
			return false;
		}

		// TODO: think about this
		data.Size = width * height * channels;

		TextureSpecification spec;
		spec.Width = width;
		spec.Height = height;
		switch (channels)
		{
		case 3:
			spec.Format = ImageFormat::RGB8;
			break;
		case 4:
			spec.Format = ImageFormat::RGBA8;
			break;
		}

		asset = Texture2D::Create(spec, data);
		data.Release();
		return true;
	}

	bool TextureSerializerAPI::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		return false;
	}

	Ref<Asset> TextureSerializerAPI::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		return Ref<Asset>();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// LevelAssetSerializerAPI
	//////////////////////////////////////////////////////////////////////////////////

	void LevelAssetSerializerAPI::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		LevelSerializer serializer(asset.As<Level>());
		serializer.Serialize(metadata.FilePath.string());
	}

	bool LevelAssetSerializerAPI::Deserialize(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<Level>::Create();
		LevelSerializer serializer(asset.As<Level>());
		// Known gap: LevelSerializer's result is ignored, so a missing or corrupt
		// .rdlvl yields an EMPTY level that reports success — levels bypass the
		// RAD-13 failure contract the texture path honors.
		serializer.Deserialize(metadata.FilePath.string());
		return true;
	}

	bool LevelAssetSerializerAPI::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Level> level = Ref<Level>::Create("AssetPackTemp", false);
		const auto& metadata = AssetManager::GetMetadata(handle);
		LevelSerializer serializer(level);
		if (serializer.Deserialize(metadata.FilePath))
		{
			return serializer.SerializeToAssetPack(stream, outInfo);
		}
		return false;
	}

	Ref<Asset> LevelAssetSerializerAPI::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		RADIANT_ASSERT(false, "Serialization path not supported for this asset type"); // Not needed
		return nullptr;
	}

	Ref<Level> LevelAssetSerializerAPI::DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo) const
	{
		Ref<Level> level = Ref<Level>::Create();
		LevelSerializer serializer(level);
		if (serializer.DeserializeFromAssetPack(stream, levelInfo))
			return level;

		return nullptr;
	}
}
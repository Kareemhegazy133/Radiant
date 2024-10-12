#include "rdpch.h"
#include "AssetSerializerAPI.h"

#include <stb_image.h>

#include "AssetManager.h"

#include "ECS/LevelSerializer.h"

namespace Radiant {

	//////////////////////////////////////////////////////////////////////////////////
	// TextureSerializerAPI
	//////////////////////////////////////////////////////////////////////////////////

	void TextureSerializerAPI::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		RADIANT_ASSERT(false); // Not needed
	}

	Ref<Asset> TextureSerializerAPI::Deserialize(const AssetMetadata& metadata) const
	{
		RADIANT_PROFILE_FUNCTION();

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		Buffer data;

		{
			RADIANT_PROFILE_SCOPE("stbi_load - TextureSerializerAPI::Deserialize");
			std::string pathStr = metadata.FilePath.string();
			data.Data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
			channels = 4;
		}

		if (data.Data == nullptr)
		{
			RADIANT_ERROR("TextureSerializerAPI::Deserialize - Could not load texture from filepath: {0}", metadata.FilePath.string());
			return nullptr;
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

		Ref<Texture2D> texture = Texture2D::Create(spec, data);
		data.Release();
		return texture;
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
		const Ref<Level> level = std::static_pointer_cast<Level>(asset);
		LevelSerializer serializer(level);
		serializer.Serialize(metadata.FilePath.string());
	}

	Ref<Asset> LevelAssetSerializerAPI::Deserialize(const AssetMetadata& metadata) const
	{
		Ref<Level> level = CreateRef<Level>();
		LevelSerializer serializer(level);
		serializer.Deserialize(metadata.FilePath.string());
		return level;
	}

	bool LevelAssetSerializerAPI::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<Level> level = CreateRef<Level>("AssetPackTemp", false);
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
		RADIANT_ASSERT(false); // Not needed
		return nullptr;
	}

	Ref<Level> LevelAssetSerializerAPI::DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo) const
	{
		Ref<Level> level = CreateRef<Level>();
		LevelSerializer serializer(level);
		if (serializer.DeserializeFromAssetPack(stream, levelInfo))
			return level;

		return nullptr;
	}
}
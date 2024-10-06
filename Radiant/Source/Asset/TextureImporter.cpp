#include "rdpch.h"
#include "TextureImporter.h"

#include "Renderer/Texture.h"

namespace Radiant {

	Ref<Texture2D> TextureImporter::ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata)
	{
		RADIANT_PROFILE_FUNCTION();

		return LoadTexture2D(metadata.FilePath);
	}

	Ref<Texture2D> TextureImporter::LoadTexture2D(const std::filesystem::path& path)
	{
		RADIANT_PROFILE_FUNCTION();

		return Texture2D::Create(path.string());
	}

}
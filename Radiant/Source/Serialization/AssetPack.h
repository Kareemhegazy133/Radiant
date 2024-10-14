#pragma once

#include <filesystem>

#include "Core/UUID.h"
#include "Asset/Asset.h"
#include "ECS/Level.h"

#include "AssetPackSerializer.h"
#include "AssetPackFile.h"

namespace Radiant {

	class AssetPack
	{
	public:
		AssetPack() = default;
		AssetPack(const std::filesystem::path& path);

		Ref<Level> LoadLevel(AssetHandle levelHandle);
		Ref<Asset> LoadAsset(AssetHandle levelHandle, AssetHandle assetHandle);

		bool IsAssetHandleValid(AssetHandle assetHandle) const;
		bool IsAssetHandleValid(AssetHandle levelHandle, AssetHandle assetHandle) const;

		Buffer ReadAppBinary();
		uint64_t GetBuildVersion();

		AssetType GetAssetType(AssetHandle levelHandle, AssetHandle assetHandle) const;

		static Ref<AssetPack> Create(const std::filesystem::path& path);
		static Ref<AssetPack> Load(const std::filesystem::path& path);
	private:
		std::filesystem::path m_Path;
		AssetPackFile m_File;

		AssetPackSerializer m_Serializer;

		std::unordered_set<AssetHandle> m_AssetHandleIndex;
	};
}
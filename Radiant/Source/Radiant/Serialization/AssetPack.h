#pragma once

#include <filesystem>

#include "Radiant/Core/UUID.h"
#include "Radiant/Asset/Asset.h"
#include "Radiant/ECS/Level.h"

#include "AssetPackSerializer.h"
#include "AssetPackFile.h"

namespace Radiant {

	/**
	 * PARKED DEAD CODE — nothing in the engine or Reaper calls this. Runtime view
	 * over a binary .rdap asset pack (index held in memory, payloads streamed on
	 * demand). Kept as the seed of the Phase 4 cook/runtime pipeline; RAD-46
	 * rebuilds it before first real use. Do not extend until then.
	 */
	class AssetPack : public RefCounted
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
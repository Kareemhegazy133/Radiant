#pragma once

#include "AssetMetadata.h"

#include "AssetSerializerAPI.h"

#include "Radiant/Serialization/FileStream.h"

#include "Radiant/ECS/Level.h"

namespace Radiant {

	/**
	 * Static facade over the per-type serializer registry: Init() maps each
	 * AssetType to an AssetSerializerAPI implementation, and every load/save
	 * dispatches through that map. This is how a new asset type plugs in — add
	 * the AssetType enum value, implement an AssetSerializerAPI subclass,
	 * register it in Init(), and map its file extension in AssetExtensions.h.
	 * Called by the AssetManager; game code never talks to it directly.
	 */
	class AssetSerializer
	{
	public:
		/** (Re)builds the type -> serializer map. Called by AssetManager::Init before any load. */
		static void Init();

		/**
		 * Dispatches to the serializer registered for metadata.Type. Returns
		 * false (with a warning) when the type has no registered serializer, or
		 * when the serializer reports failure; 'asset' is untouched in the
		 * no-serializer case.
		 */
		static bool LoadAsset(const AssetMetadata& metadata, Ref<Asset>& asset);
		/** Dispatches to the registered serializer; warns and does nothing when the type has no serializer. */
		static void SaveAsset(const AssetMetadata& metadata, const Ref<Asset>& asset);

		// Asset-pack paths: parked dead code pending the Phase 4 cook pipeline (RAD-46).
		static bool SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
		static Ref<Level> DeserializeLevelFromAssetPack(FileStreamReader& stream, const AssetPackFile::LevelInfo& levelInfo);

	private:
		static std::unordered_map<AssetType, Scope<AssetSerializerAPI>> s_Serializers;
	};

}
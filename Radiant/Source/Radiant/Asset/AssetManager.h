#pragma once

#include "Asset.h"
#include "AssetRegistry.h"
#include "Radiant/Serialization/AssetPack.h"

namespace Radiant {

	/**
	 * Static, main-thread-only asset subsystem: resolves handles to live assets,
	 * lazily deserializing on first request through the per-type serializer
	 * registry (AssetSerializer). Loaded assets are Ref-counted and cached — every
	 * GetAsset call for the same handle shares one instance, and the cache itself
	 * holds a Ref, so payloads stay alive until ClearAssets() regardless of
	 * outside references.
	 *
	 * Identity contract: runtime code references assets by AssetHandle only
	 * (0 = invalid sentinel); file paths exist solely in registry metadata and
	 * import-time code, so content can move without breaking references.
	 *
	 * Failure contract (RAD-13): GetAsset/LoadAsset return nullptr on failure and
	 * never cache or register a failed load, so a later call can retry.
	 */
	// TODO: Multi threading
	class AssetManager
	{
	public:
		/** Builds the per-type serializer registry. Call once at startup, before any load. */
		static void Init();

		/**
		 * Path-based load with import semantics: if the path is not in the
		 * registry, a fresh handle is minted and the file registered on the fly —
		 * an import operation fused into a runtime API (known design debt,
		 * RAD-43; unregistered files get a different handle every run). Returns
		 * the cached instance when already loaded. Returns nullptr on failure;
		 * failed loads are never cached or registered.
		 */
		static Ref<Asset> LoadAsset(const std::filesystem::path& filepath);

		/**
		 * Typed wrapper over LoadAsset(path); returns nullptr when the load
		 * fails. The cast to T is unchecked — requesting the wrong type is a
		 * programmer error, not a detected failure.
		 */
		template<typename T>
		static Ref<T> LoadAsset(const std::filesystem::path& filepath)
		{
			static_assert(std::is_base_of<Asset, T>::value, "LoadAsset<T> can only be used with types derived from Asset");

			Ref<Asset> asset = LoadAsset(filepath);
			if (!asset)
			{
				RADIANT_ERROR("AssetManager: Asset at {0} is not of type requested!", filepath.string());
				return nullptr;
			}
			return asset.As<T>();
		}

		/** Not implemented — parked with the AssetPack dead code pending the Phase 4 cook pipeline (RAD-46); calling it fails at link time. */
		static Ref<AssetPack> LoadAssetPack(const std::filesystem::path& filepath);

		/**
		 * Serializes a level to disk as YAML (.rdlvl) and registers/updates its
		 * metadata. Saving is an explicit editor/tool action — running the game
		 * must never modify committed assets.
		 */
		static void SaveLevel(const Ref<Level>& level, const std::filesystem::path& filepath);

		/**
		 * Typed wrapper over GetAsset(handle); returns nullptr for invalid
		 * handles and failed loads. The cast to T is unchecked — requesting the
		 * wrong type is a programmer error, not a detected failure.
		 */
		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			static_assert(std::is_base_of<Asset, T>::value, "GetAsset<T> can only be used with types derived from Asset");

			Ref<Asset> asset = GetAsset(assetHandle);

			if (!asset)
			{
				RADIANT_ERROR("AssetManager: Asset with handle {0} not found!", (uint64_t)assetHandle);
				return nullptr;
			}

			return asset.As<T>();
		}

		/**
		 * The runtime resolution path: returns the cached asset, lazily loading
		 * it on first request. Returns nullptr for handle 0, unregistered
		 * handles, and failed loads; failures are never cached (RAD-13), so the
		 * next call retries.
		 */
		static Ref<Asset> GetAsset(AssetHandle assetHandle);

		/** True when the handle is non-zero and registered; says nothing about whether the payload actually loads. */
		static bool IsAssetHandleValid(AssetHandle assetHandle);
		/** True when the payload is already in the cache. Never triggers a load. */
		static bool IsAssetLoaded(AssetHandle assetHandle);

		/**
		 * Resolves the type by loading the asset (via GetAsset) if necessary —
		 * a full payload load, not a metadata peek. Returns AssetType::None when
		 * the asset cannot be loaded.
		 */
		static AssetType GetAssetType(AssetHandle assetHandle);
		/** Scans the registry (O(n)) for all handles of the given type. Reads metadata only — loads nothing. */
		static std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

		/** Registers or overwrites a registry entry. Asserts on handle 0 or a handle/metadata mismatch. */
		static void SetMetadata(AssetHandle assetHandle, const AssetMetadata& metadata);
		/** Returns a copy of the registry record, or a null record (IsValid() == false) for unknown handles. */
		static AssetMetadata GetMetadata(AssetHandle assetHandle);
		/**
		 * Path lookup via linear registry scan — import/tooling use only; runtime
		 * code resolves by handle. Returns a shared null record (IsValid() ==
		 * false) for unregistered paths. The reference points into the registry
		 * and is invalidated by any registry mutation.
		 */
		static const AssetMetadata& GetMetadata(const std::filesystem::path& filepath);

		// Path-based helpers for import and tooling code — runtime code references
		// assets by handle only. All return the null/empty value for unknown paths.

		template<typename T>
		static Ref<T> GetAssetFromFilePath(const std::filesystem::path& filepath)
		{
			return GetAsset<T>(GetAssetHandleFromFilePath(filepath));
		}

		static Ref<Asset> GetAssetFromFilePath(const std::filesystem::path& filepath);
		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
		static std::filesystem::path GetFileSystemPath(AssetHandle assetHandle);
		static std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);

		/**
		 * Loads a .rdar registry file, merging entries into the current registry
		 * (colliding handles are overwritten). Entries with an unknown type or a
		 * zero handle are skipped with a warning; when the stored type disagrees
		 * with the file extension, the extension wins. Returns false when the
		 * file is missing or malformed.
		 */
		static bool LoadAssetRegistry(const std::filesystem::path& assetRegistryPath);
		/** Writes the registry as YAML (.rdar), forward-slash paths. An explicit editor/tool action — never called by gameplay code. */
		static void SaveAssetRegistry(const std::filesystem::path& assetRegistryPath);
		/**
		 * Clears BOTH the loaded-payload cache and the registry: outstanding Refs
		 * keep their payloads alive, but every handle becomes unresolvable until
		 * a registry is loaded again. This backs Reaper's per-game-state asset
		 * scoping (load a .rdar on state enter, clear on exit).
		 */
		static void ClearAssets();

	private:
		struct AssetManagerData
		{
			// Identity: handle -> metadata (what exists and where it lives on disk).
			AssetRegistry m_AssetRegistry;
			// Payload cache: handle -> live instance. Holding Ref keeps assets alive until ClearAssets().
			std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		};
		
		static Scope<AssetManagerData> s_AssetManagerData;
	};

}
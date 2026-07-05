#pragma once

#include "Radiant/Core/UUID.h"

#include "AssetTypes.h"

namespace Radiant {

	/**
	 * Globally unique asset identity: a random 64-bit id minted at import time.
	 * Handle 0 is the invalid sentinel. Runtime code stores and serializes handles
	 * only — file paths exist solely in registry metadata and import code, so
	 * content can move on disk without breaking references.
	 */
	using AssetHandle = UUID;

	/**
	 * Base class for everything the AssetManager can load and cache. Assets are
	 * intrusively ref-counted (always held as Ref<Asset>): the manager caches one
	 * instance per handle and hands the same Ref to every caller, so payloads are
	 * shared, never duplicated. Derived types shadow GetStaticType() and override
	 * GetAssetType() with their own AssetType so typed lookups and the per-type
	 * serializer registry can dispatch on it.
	 */
	class Asset : public RefCounted
	{
	public:

		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

	public:
		// Default-constructs to a fresh random id (UUID() is random, not zero);
		// the AssetManager stamps the registry handle over it after a successful load.
		AssetHandle Handle;
	};
}
#pragma once

#include "Asset.h"

#include "AssetMetadata.h"

namespace Radiant {

	/**
	 * In-memory catalog mapping AssetHandle -> AssetMetadata — the single source
	 * of truth for what the AssetManager can resolve. Owned by the AssetManager
	 * and persisted as a YAML .rdar file via AssetManager::Load/SaveAssetRegistry.
	 * Holds identity only: payloads live in the manager's loaded-asset cache, so
	 * clearing the registry makes handles unresolvable but unloads nothing.
	 */
	class AssetRegistry
	{
	public:
		/** Asserts the handle is present — check Contains() first; a missing handle is a programmer error. */
		AssetMetadata& Get(const AssetHandle handle);
		const AssetMetadata& Get(const AssetHandle handle) const;
		/** Inserts or overwrites. Asserts the handle is non-zero and matches metadata.Handle. */
		void Set(const AssetHandle handle, const AssetMetadata& metadata);

		size_t Count() const { return m_AssetRegistry.size(); }
		bool Contains(const AssetHandle handle) const;
		size_t Remove(const AssetHandle handle);
		void Clear();

		auto begin() { return m_AssetRegistry.begin(); }
		auto end() { return m_AssetRegistry.end(); }
		auto begin() const { return m_AssetRegistry.cbegin(); }
		auto end() const { return m_AssetRegistry.cend(); }

	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_AssetRegistry;
	};

}
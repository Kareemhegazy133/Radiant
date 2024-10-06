#pragma once

#include "Asset.h"

#include "AssetMetadata.h"

namespace Radiant {

	class AssetRegistry
	{
	public:
		AssetMetadata& Get(const AssetHandle handle);
		const AssetMetadata& Get(const AssetHandle handle) const;
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
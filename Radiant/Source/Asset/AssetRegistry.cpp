#include "rdpch.h"
#include "AssetRegistry.h"

namespace Radiant {

	AssetMetadata& AssetRegistry::Get(const AssetHandle handle)
	{
		RADIANT_ASSERT(m_AssetRegistry.find(handle) != m_AssetRegistry.end());
		return m_AssetRegistry.at(handle);
	}

	const AssetMetadata& AssetRegistry::Get(const AssetHandle handle) const
	{
		RADIANT_ASSERT(m_AssetRegistry.find(handle) != m_AssetRegistry.end());
		return m_AssetRegistry.at(handle);
	}

	void AssetRegistry::Set(const AssetHandle handle, const AssetMetadata& metadata)
	{
		RADIANT_ASSERT(metadata.Handle == handle);
		RADIANT_ASSERT(handle != 0);
		m_AssetRegistry[handle] = metadata;
	}

	bool AssetRegistry::Contains(const AssetHandle handle) const
	{
		return m_AssetRegistry.find(handle) != m_AssetRegistry.end();
	}

	size_t AssetRegistry::Remove(const AssetHandle handle)
	{
		return m_AssetRegistry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		m_AssetRegistry.clear();
	}

}
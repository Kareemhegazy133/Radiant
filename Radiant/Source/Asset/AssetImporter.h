#pragma once

#include "AssetMetadata.h"

namespace Radiant {

	class AssetImporter
	{
	public:
		static Ref<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
	};

}
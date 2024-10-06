#pragma once

#include "Core/UUID.h"

#include "AssetTypes.h"

namespace Radiant {

	using AssetHandle = UUID;

	class Asset
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
		AssetHandle Handle;
	};
}
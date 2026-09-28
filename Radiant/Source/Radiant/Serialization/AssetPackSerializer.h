#pragma once

#include <filesystem>

#include "AssetPackFile.h"

#include "Radiant/Core/Buffer.h"

namespace Radiant {

	/**
	 * PARKED DEAD CODE (RAD-46). Writes/reads the .rdap pack layout: header, a
	 * zero-filled index placeholder, payload blobs, then a seek-back pass that
	 * overwrites the placeholder with real offsets. Revived and hardened as the
	 * Phase 4 cook pipeline; its wire-format debts are catalogued under RAD-45.
	 */
	class AssetPackSerializer
	{
	public:
		static void Serialize(const std::filesystem::path& path, AssetPackFile& file, Buffer appBinary);
		static bool DeserializeIndex(const std::filesystem::path& path, AssetPackFile& file);
	private:
		static uint64_t CalculateIndexTableSize(const AssetPackFile& file);
	};

}
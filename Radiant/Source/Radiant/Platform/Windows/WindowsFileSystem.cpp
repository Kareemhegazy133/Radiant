#include "Radiant/rdpch.h"
#include "Radiant/Utilities/FileSystem.h"

namespace Radiant {

	bool FileSystem::WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer)
	{
		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);

		if (!stream)
		{
			stream.close();
			return false;
		}

		stream.write((char*)buffer.Data, buffer.Size);
		stream.close();

		return true;
	}

	Buffer FileSystem::ReadBytes(const std::filesystem::path& filepath)
	{
		// The returned Buffer owns a heap allocation the caller must Release() —
		// Buffer has no destructor
		Buffer buffer;

		// A missing/locked file is a content mistake, not a programmer error:
		// warn and return an empty (Data == nullptr) Buffer instead of asserting
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);
		if (!stream)
		{
			RADIANT_WARN("FileSystem: failed to open '{}' for reading", filepath.string());
			return buffer;
		}

		auto end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		auto size = end - stream.tellg();
		if (size <= 0)
		{
			RADIANT_WARN("FileSystem: '{}' is empty", filepath.string());
			return buffer;
		}

		buffer.Allocate((uint32_t)size);
		stream.read((char*)buffer.Data, buffer.Size);
		stream.close();

		return buffer;
	}
}
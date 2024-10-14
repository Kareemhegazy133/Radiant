#pragma once

#include <filesystem>

namespace Radiant {

	class FileSystem
	{
	public:

		static bool CreateDirectory(const std::filesystem::path& directory);
		static bool CreateDirectory(const std::string& directory);
		static bool Exists(const std::filesystem::path& filepath);
		static bool Exists(const std::string& filepath);
	};
}
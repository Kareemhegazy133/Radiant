#pragma once

#include <filesystem>

#include "Radiant/Core/Buffer.h"

namespace Radiant {

	/**
	 * Static filesystem helpers: thin std::filesystem wrappers plus whole-file
	 * byte I/O. WriteBytes/ReadBytes are implemented per platform
	 * (Platform/Windows/WindowsFileSystem.cpp); the rest live in
	 * Utilities/FileSystem.cpp.
	 */
	class FileSystem
	{
	public:

		/** Creates the directory and any missing parents. Returns false when nothing was created (including "already exists"). */
		static bool CreateDirectory(const std::filesystem::path& directory);
		static bool CreateDirectory(const std::string& directory);
		static bool Exists(const std::filesystem::path& filepath);
		static bool Exists(const std::string& filepath);

		/** Replaces the file's contents with the buffer. Returns false when the file cannot be opened. */
		static bool WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer);
		/**
		 * Reads the whole file into a freshly allocated Buffer — ownership passes
		 * to the caller, who must Release() it (Buffer is not RAII). Asserts
		 * (Debug/Release only) on a missing or empty file rather than reporting
		 * failure.
		 */
		static Buffer ReadBytes(const std::filesystem::path& filepath);
	};
}
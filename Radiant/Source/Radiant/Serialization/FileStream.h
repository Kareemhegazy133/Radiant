#pragma once

#include "StreamWriter.h"
#include "StreamReader.h"
#include "Radiant/Core/Buffer.h"

#include <filesystem>
#include <fstream>

namespace Radiant
{
	//==============================================================================
	/// FileStreamWriter
	/**
	 * StreamWriter over a std::ofstream. Opens the file in binary mode,
	 * truncating existing content; the stream closes on destruction.
	 * Non-copyable — it owns the file handle. Check IsStreamGood() (or the bool
	 * conversion) after construction: an unopenable path is not otherwise
	 * reported.
	 */
	class FileStreamWriter : public StreamWriter
	{
	public:
		FileStreamWriter(const std::filesystem::path& path);
		FileStreamWriter(const FileStreamWriter&) = delete;
		virtual ~FileStreamWriter();

		bool IsStreamGood() const final { return m_Stream.good(); }
		uint64_t GetStreamPosition() final { return m_Stream.tellp(); }
		void SetStreamPosition(uint64_t position) final { m_Stream.seekp(position); }
		bool WriteData(const char* data, size_t size) final;

	private:
		std::filesystem::path m_Path;
		std::ofstream m_Stream;
	};

	//==============================================================================
	/// FileStreamReader
	/**
	 * StreamReader over a std::ifstream, opened in binary mode and closed on
	 * destruction. Non-copyable — it owns the file handle. Check IsStreamGood()
	 * (or the bool conversion) after construction: a missing file is not
	 * otherwise reported.
	 */
	class FileStreamReader : public StreamReader
	{
	public:
		FileStreamReader(const std::filesystem::path& path);
		FileStreamReader(const FileStreamReader&) = delete;
		~FileStreamReader();

		bool IsStreamGood() const final { return m_Stream.good(); }
		uint64_t GetStreamPosition() override { return m_Stream.tellg(); }
		void SetStreamPosition(uint64_t position) override { m_Stream.seekg(position); }
		bool ReadData(char* destination, size_t size) override;

	private:
		std::filesystem::path m_Path;
		std::ifstream m_Stream;
	};

} // namespace Radiant
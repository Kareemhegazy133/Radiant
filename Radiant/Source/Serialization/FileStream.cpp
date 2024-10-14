#include "rdpch.h"
#include "FileStream.h"

namespace Radiant
{
	//==============================================================================
	/// FileStreamWriter
	FileStreamWriter::FileStreamWriter(const std::filesystem::path& path)
		: m_Path(path)
	{
		m_Stream = std::ofstream(path, std::ofstream::out | std::ofstream::binary);
	}

	FileStreamWriter::~FileStreamWriter()
	{
		m_Stream.close();
	}

	bool FileStreamWriter::WriteData(const char* data, size_t size)
	{
		m_Stream.write(data, size);
		return true;
	}

	//==============================================================================
	/// FileStreamReader
	FileStreamReader::FileStreamReader(const std::filesystem::path& path)
		: m_Path(path)
	{
		m_Stream = std::ifstream(path, std::ifstream::in | std::ifstream::binary);
	}

	FileStreamReader::~FileStreamReader()
	{
		m_Stream.close();
	}

	bool FileStreamReader::ReadData(char* destination, size_t size)
	{
		m_Stream.read(destination, size);
		return true;
	}

} // namespace Radiant
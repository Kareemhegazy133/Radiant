#include "Radiant/rdpch.h"
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
		// Ignores stream state and unconditionally reports success — a failed write
		// is invisible to callers, and the asserts layered on this in WriteRaw can
		// never fire. RAD-45 threads real stream status through this primitive.
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
		// Ignores stream state and unconditionally reports success — a truncated or
		// corrupt file reads garbage that callers cannot detect, and the asserts
		// layered on this in ReadRaw can never fire. RAD-45 threads real stream
		// status through this primitive.
		m_Stream.read(destination, size);
		return true;
	}

} // namespace Radiant
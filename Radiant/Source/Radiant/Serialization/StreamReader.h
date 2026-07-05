#pragma once

#include "Radiant/Core/Buffer.h"

namespace Radiant
{
	/**
	 * Abstract binary stream reader — the mirror of StreamWriter: typed helpers
	 * layered on a single primitive, ReadData. Implementations (see
	 * FileStreamReader) supply position control and the raw read. Every helper
	 * must consume exactly what its StreamWriter counterpart produced; there is
	 * no framing or validation in the format to catch drift.
	 */
	class StreamReader
	{
	public:
		virtual ~StreamReader() = default;

		virtual bool IsStreamGood() const = 0;
		virtual uint64_t GetStreamPosition() = 0;
		virtual void SetStreamPosition(uint64_t position) = 0;
		virtual bool ReadData(char* destination, size_t size) = 0;

		operator bool() const { return IsStreamGood(); }

		/**
		 * Reads a uint64_t size prefix when 'size' is 0, otherwise trusts the
		 * caller-supplied byte count. Allocates the buffer's storage — ownership
		 * passes to the caller, who must Release() it (Buffer is not RAII).
		 */
		void ReadBuffer(Buffer& buffer, uint32_t size = 0);
		/** Length-prefixed string. The prefix is currently size_t — ABI-dependent wire format; RAD-45 pins it to uint64_t. */
		void ReadString(std::string& string);

		/** memcpy into T's object representation — trivially copyable types only. */
		template<typename T>
		void ReadRaw(T& type)
		{
			bool success = ReadData((char*)&type, sizeof(T));
			RADIANT_ASSERT(success);
		}

		/**
		 * Deserializes via the static-method contract: T must provide
		 * static void Deserialize(StreamReader*, T&) — the mirror of
		 * StreamWriter::WriteObject.
		 */
		template<typename T>
		void ReadObject(T& obj)
		{
			T::Deserialize(this, obj);
		}

		/**
		 * Reads a uint32_t element count when 'size' is 0 (a count supplied
		 * out-of-band skips the prefix), then each key/value — ReadRaw for
		 * trivial types, ReadObject otherwise, matching WriteMap exactly.
		 */
		template<typename Key, typename Value>
		void ReadMap(std::map<Key, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			for (uint32_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (std::is_trivial<Key>())
					ReadRaw<Key>(key);
				else
					ReadObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					ReadRaw<Value>(map[key]);
				else
					ReadObject<Value>(map[key]);
			}
		}

		template<typename Key, typename Value>
		void ReadMap(std::unordered_map<Key, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			for (uint32_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (std::is_trivial<Key>())
					ReadRaw<Key>(key);
				else
					ReadObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					ReadRaw<Value>(map[key]);
				else
					ReadObject<Value>(map[key]);
			}
		}

		template<typename Value>
		void ReadMap(std::unordered_map<std::string, Value>& map, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			for (uint32_t i = 0; i < size; i++)
			{
				std::string key;
				ReadString(key);

				if constexpr (std::is_trivial<Value>())
					ReadRaw<Value>(map[key]);
				else
					ReadObject<Value>(map[key]);
			}
		}

		/** Same layout rules as ReadMap: uint32_t count (read when 'size' is 0), then elements via ReadRaw/ReadObject. */
		template<typename T>
		void ReadArray(std::vector<T>& array, uint32_t size = 0)
		{
			if (size == 0)
				ReadRaw<uint32_t>(size);

			array.resize(size);

			for (uint32_t i = 0; i < size; i++)
			{
				if constexpr (std::is_trivial<T>())
					ReadRaw<T>(array[i]);
				else
					ReadObject<T>(array[i]);
			}
		}

	};

	template<>
	inline void StreamReader::ReadArray(std::vector<std::string>& array, uint32_t size)
	{
		if (size == 0)
			ReadRaw<uint32_t>(size);

		array.resize(size);

		for (uint32_t i = 0; i < size; i++)
			ReadString(array[i]);
	}
} // namespace Radiant
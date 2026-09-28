#pragma once

#include "Radiant/Core/Buffer.h"

#include <map>

namespace Radiant
{
	/**
	 * Abstract binary stream writer: typed helpers layered on a single primitive,
	 * WriteData. Implementations (see FileStreamWriter) supply position control
	 * and the raw write. The wire format is declared little-endian and is only
	 * ever read back by StreamReader — the helper pairs on both sides must stay
	 * symmetric or the format silently diverges.
	 */
	class StreamWriter
	{
	public:
		virtual ~StreamWriter() = default;

		virtual bool IsStreamGood() const = 0;
		virtual uint64_t GetStreamPosition() = 0;
		virtual void SetStreamPosition(uint64_t position) = 0;
		virtual bool WriteData(const char* data, size_t size) = 0;

		operator bool() const { return IsStreamGood(); }

		/** Writes buffer.Size as a uint64_t prefix (unless writeSize is false), then the raw bytes. */
		void WriteBuffer(Buffer buffer, bool writeSize = true);
		/** Writes 'size' zero bytes — used to reserve index space that a later SetStreamPosition pass overwrites (see AssetPackSerializer). */
		void WriteZero(uint64_t size);
		/** Length-prefixed string. The prefix is currently size_t — ABI-dependent wire format; RAD-45 pins it to uint64_t. */
		void WriteString(const std::string& string);

		/** memcpy of T's object representation — trivially copyable types only; padding bytes are written as-is. */
		template<typename T>
		void WriteRaw(const T& type)
		{
			bool success = WriteData((char*)&type, sizeof(T));
			RADIANT_ASSERT(success);
		}

		/**
		 * Serializes via the static-method contract: T must provide
		 * static void Serialize(StreamWriter*, const T&). Keeps serializable
		 * types vtable-free so plain structs can participate.
		 */
		template<typename T>
		void WriteObject(const T& obj)
		{
			T::Serialize(this, obj);
		}

		/**
		 * Writes an optional uint32_t element count, then each key/value —
		 * WriteRaw for trivial types, WriteObject otherwise. The triviality
		 * split must match the ReadMap side exactly.
		 */
		template<typename Key, typename Value>
		void WriteMap(const std::map<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)map.size());

			for (const auto& [key, value] : map)
			{
				if constexpr (std::is_trivial<Key>())
					WriteRaw<Key>(key);
				else
					WriteObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					WriteRaw<Value>(value);
				else
					WriteObject<Value>(value);
			}
		}

		template<typename Key, typename Value>
		void WriteMap(const std::unordered_map<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)map.size());

			for (const auto& [key, value] : map)
			{
				if constexpr (std::is_trivial<Key>())
					WriteRaw<Key>(key);
				else
					WriteObject<Key>(key);

				if constexpr (std::is_trivial<Value>())
					WriteRaw<Value>(value);
				else
					WriteObject<Value>(value);
			}
		}

		template<typename Value>
		void WriteMap(const std::unordered_map<std::string, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)map.size());

			for (const auto& [key, value] : map)
			{
				WriteString(key);

				if constexpr (std::is_trivial<Value>())
					WriteRaw<Value>(value);
				else
					WriteObject<Value>(value);
			}
		}

		/** Same layout rules as WriteMap: optional uint32_t count, then elements via WriteRaw/WriteObject. */
		template<typename T>
		void WriteArray(const std::vector<T>& array, bool writeSize = true)
		{
			if (writeSize)
				WriteRaw<uint32_t>((uint32_t)array.size());

			for (const auto& element : array)
			{
				if constexpr (std::is_trivial<T>())
					WriteRaw<T>(element);
				else
					WriteObject<T>(element);
			}
		}

	};

	template<>
	inline void StreamWriter::WriteArray(const std::vector<std::string>& array, bool writeSize)
	{
		if (writeSize)
			WriteRaw<uint32_t>((uint32_t)array.size());

		for (const auto& element : array)
			WriteString(element);
	}
} // namespace Radiant
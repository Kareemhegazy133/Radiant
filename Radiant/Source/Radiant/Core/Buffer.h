#pragma once

#include "Radiant/Core/Assert.h"

namespace Radiant {

	/**
	 * Raw memory block with MANUAL lifetime: there is no destructor — whoever
	 * owns the allocation must call Release() (or use BufferSafe). Copies and
	 * the (data, size) constructor are shallow, non-owning views; Allocate()
	 * and Copy() produce blocks the holder is responsible for freeing. Size is
	 * in bytes. Bounds asserts compile out in Dist.
	 */
	struct Buffer
	{
		void* Data = nullptr;
		uint64_t Size = 0;

		Buffer() = default;

		Buffer(const void* data, uint64_t size = 0)
			: Data((void*)data), Size(size) { }

		static Buffer Copy(const Buffer& other)
		{
			Buffer buffer;
			buffer.Allocate(other.Size);
			memcpy(buffer.Data, other.Data, other.Size);
			return buffer;
		}

		static Buffer Copy(const void* data, uint64_t size)
		{
			Buffer buffer;
			buffer.Allocate(size);
			memcpy(buffer.Data, data, size);
			return buffer;
		}

		void Allocate(uint64_t size)
		{
			delete[](byte*)Data;
			Data = nullptr;
			Size = size;

			if (size == 0)
				return;

			Data = new byte[size];
		}

		void Release()
		{
			delete[](byte*)Data;
			Data = nullptr;
			Size = 0;
		}

		void ZeroInitialize()
		{
			if (Data)
				memset(Data, 0, Size);
		}

		/** Reinterprets the bytes at offset as a T. No bounds checking — unlike Write/ReadBytes, an invalid offset is never caught. */
		template<typename T>
		T& Read(uint64_t offset = 0)
		{
			return *(T*)((byte*)Data + offset);
		}

		template<typename T>
		const T& Read(uint64_t offset = 0) const
		{
			return *(T*)((byte*)Data + offset);
		}

		/** Heap-copies [offset, offset + size). Ownership transfers: the caller must delete[] the returned array. */
		byte* ReadBytes(uint64_t size, uint64_t offset) const
		{
			RADIANT_ASSERT(offset + size <= Size, "Buffer overflow!");
			byte* buffer = new byte[size];
			memcpy(buffer, (byte*)Data + offset, size);
			return buffer;
		}

		void Write(const void* data, uint64_t size, uint64_t offset = 0)
		{
			RADIANT_ASSERT(offset + size <= Size, "Buffer overflow!");
			memcpy((byte*)Data + offset, data, size);
		}

		operator bool() const
		{
			return (bool)Data;
		}

		byte& operator[](int index)
		{
			return ((byte*)Data)[index];
		}

		byte operator[](int index) const
		{
			return ((byte*)Data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)Data;
		}

		inline uint64_t GetSize() const { return Size; }
	};

	/**
	 * Buffer that frees its memory on destruction. CAUTION: copy/move are not
	 * controlled — copying one yields two owners of the same block (double
	 * free), and Copy() returning by value relies on copy elision. Treat it as
	 * a scoped local only.
	 */
	struct BufferSafe : public Buffer
	{
		~BufferSafe()
		{
			Release();
		}

		static BufferSafe Copy(const void* data, uint64_t size)
		{
			BufferSafe buffer;
			buffer.Allocate(size);
			memcpy(buffer.Data, data, size);
			return buffer;
		}
	};
}
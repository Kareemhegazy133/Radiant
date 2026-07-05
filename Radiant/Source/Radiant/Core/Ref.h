#pragma once

#include "Radiant/Core/Base.h"

#include <atomic>
#include <cstddef>
#include <type_traits>

namespace Radiant {

	class RefCounted
	{
	public:
		virtual ~RefCounted() = default;

		void IncRefCount() const
		{
			++m_RefCount;
		}

		// Decrements the refcount and returns true if this call released the last
		// reference. The release decision must come from the decrement itself:
		// fetch_sub returns the previous value atomically, so exactly one caller
		// can observe 1. A separate GetRefCount() check after decrementing is a
		// race (two threads can both see 0 and double-delete).
		bool DecRefCount() const
		{
			uint32_t previous = m_RefCount.fetch_sub(1, std::memory_order_acq_rel);

			// previous == 0 means the object was released more times than it was referenced
			if (previous == 0)
				RADIANT_DEBUGBREAK();

			return previous == 1;
		}

		// Diagnostics only — never build release logic on an observed count; it can
		// change between the load and any decision made from it.
		uint32_t GetRefCount() const { return m_RefCount.load(); }
	private:
		mutable std::atomic<uint32_t> m_RefCount = 0;
	};

#if RADIANT_TRACK_REFERENCES
	namespace RefUtils {
		void AddToLiveReferences(void* instance);
		void RemoveFromLiveReferences(void* instance);
	}
#endif

	template<typename T>
	class Ref
	{
	public:
		Ref()
			: m_Instance(nullptr)
		{
		}

		Ref(std::nullptr_t n)
			: m_Instance(nullptr)
		{
		}

		Ref(T* instance)
			: m_Instance(instance)
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		Ref(const Ref<T2>& other)
		{
			m_Instance = (T*)other.m_Instance;
			IncRef();
		}

		template<typename T2>
		Ref(Ref<T2>&& other)
		{
			m_Instance = (T*)other.m_Instance;
			other.m_Instance = nullptr;
		}

		~Ref()
		{
			DecRef();
		}

		Ref(const Ref<T>& other)
			: m_Instance(other.m_Instance)
		{
			IncRef();
		}

		Ref& operator=(std::nullptr_t)
		{
			DecRef();
			m_Instance = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other)
		{
			if (this == &other)
				return *this;

			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		Ref& operator=(const Ref<T2>& other)
		{
			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		Ref& operator=(Ref<T2>&& other)
		{
			DecRef();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		operator bool() { return m_Instance != nullptr; }
		operator bool() const { return m_Instance != nullptr; }

		T* operator->() { return m_Instance; }
		const T* operator->() const { return m_Instance; }

		T& operator*() { return *m_Instance; }
		const T& operator*() const { return *m_Instance; }

		T* Raw() { return  m_Instance; }
		const T* Raw() const { return  m_Instance; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			m_Instance = instance;
		}

		template<typename T2>
		Ref<T2> As() const
		{
			return Ref<T2>(*this);
		}

		template<typename... Args>
		static Ref<T> Create(Args&&... args)
		{
			return Ref<T>(new T(std::forward<Args>(args)...));
		}

		bool operator==(const Ref<T>& other) const
		{
			return m_Instance == other.m_Instance;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const Ref<T>& other)
		{
			if (!m_Instance || !other.m_Instance)
				return false;

			return *m_Instance == *other.m_Instance;
		}
	private:
		void IncRef() const
		{
			if (m_Instance)
			{
				m_Instance->IncRefCount();
#if RADIANT_TRACK_REFERENCES
				RefUtils::AddToLiveReferences((void*)m_Instance);
#endif
			}
		}

		void DecRef() const
		{
			if (m_Instance)
			{
				if (m_Instance->DecRefCount())
				{
					// bookkeeping first: a destroyed object must never still be tracked as live
#if RADIANT_TRACK_REFERENCES
					RefUtils::RemoveFromLiveReferences((void*)m_Instance);
#endif
					delete m_Instance;
					m_Instance = nullptr;
				}
			}
		}

		template<class T2>
		friend class Ref;
		mutable T* m_Instance;
	};

}
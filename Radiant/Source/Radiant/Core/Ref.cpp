#include "Radiant/rdpch.h"

#include "Radiant/Core/Ref.h"

#if RADIANT_TRACK_REFERENCES

#include <mutex>
#include <unordered_set>

namespace Radiant {

	static std::unordered_set<void*> s_LiveReferences;
	static std::mutex s_LiveReferenceMutex;

	namespace RefUtils {

		void AddToLiveReferences(void* instance)
		{
			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			RADIANT_ASSERT(instance);
			s_LiveReferences.insert(instance);
		}

		void RemoveFromLiveReferences(void* instance)
		{
			std::scoped_lock<std::mutex> lock(s_LiveReferenceMutex);
			RADIANT_ASSERT(instance);
			RADIANT_ASSERT(s_LiveReferences.find(instance) != s_LiveReferences.end());
			s_LiveReferences.erase(instance);
		}
	}

}

#endif

#include "rdpch.h"
#include "UUID.h"

#include <random>

#include <unordered_map>

namespace Radiant {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Radiant(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	UUID::UUID()
		: m_UUID(s_UniformDistribution(s_Radiant))
	{
	}

	UUID::UUID(uint64_t uuid)
		: m_UUID(uuid)
	{
	}

}
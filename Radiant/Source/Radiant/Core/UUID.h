#pragma once

namespace Radiant {

	/**
	 * 64-bit random identifier (not RFC 4122). Default construction draws from
	 * a global Mersenne Twister seeded once per run; uniqueness is
	 * probabilistic — there is no collision check. Value type: copy freely;
	 * hashes by value for unordered containers.
	 */
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Radiant::UUID>
	{
		std::size_t operator()(const Radiant::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}
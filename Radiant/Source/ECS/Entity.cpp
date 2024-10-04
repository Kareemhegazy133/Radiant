#include "rdpch.h"
#include "Level.h"

namespace Radiant {

	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && m_Level && m_Level->m_Registry.valid(m_EntityHandle);
	}

	Entity::operator bool() const { return IsValid(); }
}
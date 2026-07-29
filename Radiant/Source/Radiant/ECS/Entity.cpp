#include "Radiant/rdpch.h"
#include "Level.h"

namespace Radiant {

	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && m_Level && m_Level->m_Registry.valid(m_EntityHandle);
	}

	Entity::operator bool() const { return IsValid(); }

	// Zero-logic forwarders — the one implementation (and its validity
	// contract) lives in Level::Teleport; only the null-level handle must be
	// caught here, since it cannot reach the Level to be warned about
	void Entity::Teleport(const glm::vec3& translation, float rotationZ)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: Teleport called on a handle with no level");
			return;
		}
		m_Level->Teleport(*this, translation, rotationZ);
	}

	void Entity::Teleport(const glm::vec3& translation)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: Teleport called on a handle with no level");
			return;
		}
		m_Level->Teleport(*this, translation);
	}
}
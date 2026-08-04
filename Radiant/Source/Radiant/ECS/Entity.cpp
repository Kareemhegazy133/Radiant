#include "Radiant/rdpch.h"
#include "Level.h"

#include "Radiant/Gameplay/PhysicsBody.h"
#include "Radiant/Gameplay/GameplayLevel.h"

namespace Radiant {

	// Defined here, not inline in the header: PhysicsBody is returned by value,
	// so this needs the complete type, and PhysicsBody.h includes Entity.h
	PhysicsBody Entity::GetPhysicsBody() const
	{
		return PhysicsBody(*this);
	}

	// Same reason, one layer up: GameplayLevel.h includes Level.h, which
	// includes Entity.h. No guard — a null m_Level produces a handle that is
	// false, and GameplayLevel's own Resolve reports whichever verb is then
	// called on it. Warning here as well would say it twice.
	GameplayLevel Entity::GetLevel() const
	{
		return GameplayLevel(m_Level);
	}

	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && m_Level && m_Level->m_Registry.valid(m_EntityHandle);
	}

	Entity::operator bool() const { return IsValid(); }

	// Zero-logic forwarders — the one implementation (and its validity
	// contract) lives in Level::Teleport; only the null-level handle must be
	// caught here, since it cannot reach the Level to be warned about
	void Entity::Teleport(const glm::vec3& translation, float rotationZ, TeleportType teleportType)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: Teleport called on a handle with no level");
			return;
		}
		m_Level->Teleport(*this, translation, rotationZ, teleportType);
	}

	void Entity::Teleport(const glm::vec3& translation, TeleportType teleportType)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: Teleport called on a handle with no level");
			return;
		}
		m_Level->Teleport(*this, translation, teleportType);
	}

	void Entity::Destroy()
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: Destroy called on a handle with no level");
			return;
		}
		m_Level->DestroyEntity(*this);
	}

	void Entity::RefreshCollider()
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: RefreshCollider called on a handle with no level");
			return;
		}
		m_Level->RefreshCollider(*this);
	}
}
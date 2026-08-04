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

	// The GETTERS carry a full IsValid() check, unlike the forwarders below,
	// and the difference is the same principle rather than an inconsistency:
	// one guard per layer. A forwarder can delegate everything but the
	// level-less handle to Level, which reports it; a getter reads the
	// component itself, so it IS the only layer — and GetComponent asserts on a
	// dead handle, which is not the recover-and-warn contract gameplay gets
	// everywhere else on this surface.
	glm::vec3 Entity::GetLocation() const
	{
		if (!IsValid())
		{
			RADIANT_WARN("Entity: {0} called on an invalid handle", __func__);
			return glm::vec3(0.0f);
		}
		return GetComponent<TransformComponent>().Translation;
	}

	float Entity::GetRotation() const
	{
		if (!IsValid())
		{
			RADIANT_WARN("Entity: {0} called on an invalid handle", __func__);
			return 0.0f;
		}
		return GetComponent<TransformComponent>().Rotation.z;
	}

	// Zero-logic forwarders — the one implementation (and its validity
	// contract) lives in Level; only the null-level handle must be caught here,
	// since it cannot reach the Level to be warned about. __func__ rather than
	// a literal for the same reason as the physics resolvers: the name is the
	// whole value of the line, and a literal drifts silently on a rename.
	void Entity::SetLocation(const glm::vec3& translation)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->SetLocation(*this, translation);
	}

	void Entity::SetRotation(float radians)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->SetRotation(*this, radians);
	}

	void Entity::Teleport(const glm::vec3& translation, float rotationZ, TeleportType teleportType)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->Teleport(*this, translation, rotationZ, teleportType);
	}

	void Entity::Teleport(const glm::vec3& translation, TeleportType teleportType)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->Teleport(*this, translation, teleportType);
	}

	void Entity::Destroy()
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->DestroyEntity(*this);
	}

	void Entity::RefreshCollider()
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->RefreshCollider(*this);
	}
}
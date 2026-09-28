#include "Radiant/rdpch.h"
#include "Level.h"

#include "Radiant/Gameplay/PhysicsBody.h"
#include "Radiant/Gameplay/GameplayLevel.h"

namespace Radiant {

	// Out of line for the usual reason, and note what it does NOT need: iterating
	// Scopes and calling .get() works on an incomplete EntityBehaviour, and
	// IsBehaviourDetached takes a reference, so this TU never includes
	// Gameplay/EntityBehaviour.h. Reading the flag directly would have required it —
	// and friendship this class does not have.
	std::vector<EntityBehaviour*> Entity::GetBehaviours() const
	{
		std::vector<EntityBehaviour*> behaviours;

		// A query: an unusable handle is an answer, not a mistake (playbook §10)
		if (!IsValid())
			return behaviours;

		auto it = m_Level->m_Behaviours.find(m_EntityHandle);
		if (it == m_Level->m_Behaviours.end())
			return behaviours;

		const std::vector<Scope<EntityBehaviour>>& list = it->second;
		behaviours.reserve(list.size());

		for (const Scope<EntityBehaviour>& held : list)
		{
			// Skipped so this agrees with GetBehaviour<T>: a detached behaviour has
			// already had its OnDestroy and is invisible to observers until the reap
			if (held && !m_Level->IsBehaviourDetached(*held))
				behaviours.push_back(held.get());
		}

		return behaviours;
	}

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

	// Two questions, deliberately separate. The registry answers whether the row
	// still exists; IsPendingDestroy answers whether it has been condemned and is
	// merely awaiting the reap (RAD-97). Composing them HERE, in the one place
	// every consumer already asks, is what makes the whole engine and every game
	// pending-aware without a single call site changing: Entity is a value handle
	// that re-asks at each use rather than caching an answer, so validity is a
	// property of the world, not of the moment somebody looked it up. That is
	// also what lets DispatchContactEvents resolve each participant once per
	// event instead of once per callback.
	bool Entity::IsValid() const
	{
		return (m_EntityHandle != entt::null) && m_Level && m_Level->m_Registry.valid(m_EntityHandle)
			&& !m_Level->IsPendingDestroy(m_EntityHandle);
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
	void Entity::SetTransform(const glm::vec3& translation, float radians)
	{
		if (!m_Level)
		{
			RADIANT_WARN("Entity: {0} called on a handle with no level", __func__);
			return;
		}
		m_Level->SetTransform(*this, translation, radians);
	}

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
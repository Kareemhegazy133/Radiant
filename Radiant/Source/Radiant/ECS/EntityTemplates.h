#pragma once

// GetBehaviour's constraint. This header has always relied on being included at
// the end of Level.h for its declarations; the trait is its own dependency, so
// it names it rather than inheriting one transitively from entt.
#include <type_traits>

namespace Radiant {

	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		RADIANT_ASSERT(!HasComponent<T>(), "Entity already has component!");
		T& component = m_Level->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T, typename... Args>
	T& Entity::AddOrReplaceComponent(Args&&... args)
	{
		T& component = m_Level->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T>
	T& Entity::GetComponent()
	{
		RADIANT_ASSERT(HasComponent<T>(), "Entity does not have component!");
		return m_Level->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	const T& Entity::GetComponent() const
	{
		RADIANT_ASSERT(HasComponent<T>(), "Entity does not have component!");
		return m_Level->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	T* Entity::TryGetComponent()
	{
		RADIANT_ASSERT(IsValid(), "Component access on an invalid entity handle");
		return m_Level->m_Registry.try_get<T>(m_EntityHandle);
	}

	template<typename T>
	const T* Entity::TryGetComponent() const
	{
		RADIANT_ASSERT(IsValid(), "Component access on an invalid entity handle");
		return m_Level->m_Registry.try_get<T>(m_EntityHandle);
	}

	template<typename... T>
	bool Entity::HasComponent()
	{
		RADIANT_ASSERT(IsValid(), "Component access on an invalid entity handle");
		return m_Level->m_Registry.all_of<T...>(m_EntityHandle);
	}

	template<typename... T>
	bool Entity::HasComponent() const
	{
		RADIANT_ASSERT(IsValid(), "Component access on an invalid entity handle");
		return m_Level->m_Registry.all_of<T...>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		RADIANT_ASSERT(HasComponent<T>(), "Entity does not have component!");
		m_Level->m_Registry.remove<T>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponentIfExists()
	{
		RADIANT_ASSERT(IsValid(), "Component access on an invalid entity handle");
		if(HasComponent<T>())
			RemoveComponent<T>();
	}

	template<typename T>
	T* Entity::GetBehaviour() const
	{
		// EntityBehaviour is only forward-declared where this is DECLARED, and
		// complete at every real call site - by two different routes, because
		// one does not cover both. A SUBCLASS T cannot be complete without its
		// base being complete; the base-type query GetBehaviour<EntityBehaviour>()
		// has no such implication and is reachable only from a TU that included
		// the base header, which Radiant.h exports to game code. That is why
		// ECS/ never includes Gameplay/EntityBehaviour.h - doing so to "fix" an
		// error here would invert the module dependency (Gameplay -> ECS, never
		// back).
		static_assert(std::is_base_of_v<EntityBehaviour, T>,
			"Entity::GetBehaviour<T> requires T to derive from EntityBehaviour");

		// IsValid FIRST, and not for tidiness: TryGetComponent below ASSERTS on
		// a dead handle, but a dead handle is a normal answer to this question
		// (Entity.h) - a collision partner may already be gone. Delegating would
		// make the documented path fire an assert in Debug and Release while
		// passing in Dist, which is the worst possible place to differ.
		if (!IsValid())
			return nullptr;

		const NativeScriptComponent* script = TryGetComponent<NativeScriptComponent>();
		if (!script || !script->Instance)
			return nullptr;

		// The checked cast, and the whole point of the verb: null when the
		// instance is not a T. The unchecked static_cast this replaces would
		// reinterpret a live object of some other type and write through T's
		// member offsets into whatever actually sits there.
		return dynamic_cast<T*>(script->Instance);
	}

}
#pragma once

// GetBehaviour's constraint. This header has always relied on being included at
// the end of Level.h for its declarations; the trait is its own dependency, so
// it names it rather than inheriting one transitively from entt.
#include <type_traits>

namespace Radiant {

	// The engine-private gate (RAD-97). Applied to the three MUTATING accessors
	// only — reads stay open, because "is this entity pending destruction?" is a
	// fair question and IsValid() already answers the useful form of it.
	//
	// This costs the engine nothing: Level writes both gated components through
	// m_Registry directly (the mark's emplace, the snapshot pass's
	// emplace_or_replace/remove), never through this handle. If a legitimate
	// engine path ever trips this, the fix is to use m_Registry — not to widen
	// the gate.
#define RADIANT_REJECT_ENGINE_COMPONENT(T)                                              \
	static_assert(!IsEngineComponent<T>::value,                                         \
		"This component is engine-private: the engine owns it and game code must not "  \
		"add, replace or remove it. To destroy an entity use Entity::Destroy(); to "    \
		"attach gameplay behaviour use Entity::AddBehaviour<T>(); render snapshots are " \
		"maintained by Level::OnFixedUpdate. See IsEngineComponent in Components.h "    \
		"for why the type is visible but not writable.")

	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		RADIANT_REJECT_ENGINE_COMPONENT(T);
		RADIANT_ASSERT(!HasComponent<T>(), "Entity already has component!");
		T& component = m_Level->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T, typename... Args>
	T& Entity::AddOrReplaceComponent(Args&&... args)
	{
		RADIANT_REJECT_ENGINE_COMPONENT(T);
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
		// RemoveComponentIfExists inherits the gate through this function
		RADIANT_REJECT_ENGINE_COMPONENT(T);
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

		return m_Level->FindBehaviour<T>(m_EntityHandle);
	}

	// Lives here rather than in Level.h because dynamic_cast<T*> needs the
	// complete EntityBehaviour that Level.h only forward-declares — the same
	// late-completeness trick as GetBehaviour above, and legal for the same
	// reason: the cast is DEPENDENT on T, so it is checked at instantiation,
	// where a complete T implies a complete base.
	//
	// That rule is also the trap. The detached check cannot be written as a
	// direct member read here, because Scope<EntityBehaviour> does not depend on
	// T, making the access non-dependent and therefore checked the moment this
	// header is parsed. Hence IsBehaviourDetached (see Level.h).
	//
	// A Level member in an Entity-named header, for an access reason: Level is
	// EntityBehaviour's only friend.
	template<typename T>
	T* Level::FindBehaviour(entt::entity handle) const
	{
		static_assert(std::is_base_of_v<EntityBehaviour, T>,
			"Level::FindBehaviour<T> requires T to derive from EntityBehaviour");

		// A linear scan of a handful of entries, which is the same shape UE uses —
		// FindComponentByClass walks OwnedComponents and breaks on first match
		// (Actor.cpp:3991-4008). Ours is TOTAL rather than first-of-many, because
		// duplicates are rejected at attach (D5), so "the T" is a true phrase.
		if (auto it = m_Behaviours.find(handle); it != m_Behaviours.end())
		{
			for (const Scope<EntityBehaviour>& held : it->second)
			{
				// A detached behaviour is gone to observers even though the reap
				// has not freed it yet: its OnDestroy already ran, so returning it
				// would be the same ordering violation as updating it (D12).
				// Through the helper, not the member: see IsBehaviourDetached in
				// Level.h for why a direct read here would not compile.
				if (IsBehaviourDetached(*held))
					continue;

				// The checked cast, and the whole point of the verb: null when the
				// instance is not a T. The unchecked static_cast this replaces
				// would reinterpret a live object of some other type and write
				// through T's member offsets into whatever actually sits there.
				if (T* typed = dynamic_cast<T*>(held.get()))
					return typed;
			}
		}

		return nullptr;
	}

	template<typename T, typename... Args>
	T* Entity::AddBehaviour(Args&&... args)
	{
		static_assert(std::is_base_of_v<EntityBehaviour, T>,
			"Entity::AddBehaviour<T> requires T to derive from EntityBehaviour");

		// A VERB, so this warns where GetBehaviour answers null in silence: the
		// caller asked for an action on something it believed existed (playbook §10)
		if (!IsValid())
		{
			RADIANT_WARN("Entity::AddBehaviour: invalid entity handle - behaviour not attached");
			return nullptr;
		}

		// Checked BEFORE constructing, so a rejected attach allocates nothing
		T* existing = GetBehaviour<T>();
		RADIANT_ASSERT(!existing, "Entity::AddBehaviour: entity already has a behaviour of this type - duplicates are rejected so GetBehaviour<T> stays singular");
		if (existing)
		{
			// Return the EXISTING one rather than a second instance: Dist has no
			// assert, and running with two of something the query promises is
			// singular is worse than refusing the attach.
			RADIANT_WARN("Entity::AddBehaviour: duplicate behaviour type - returning the existing instance");
			return existing;
		}

		Scope<T> instance = CreateScope<T>(std::forward<Args>(args)...);
		T* observer = instance.get();

		// Constructed here, FILED by the Level: the type-specific half is
		// templated, the bookkeeping half (table + tag + walk order) must stay in
		// one non-templated place or the single-writer invariant is per-type
		m_Level->AttachBehaviour(m_EntityHandle, std::move(instance));
		return observer;
	}

	template<typename T>
	bool Entity::HasBehaviour() const
	{
		return GetBehaviour<T>() != nullptr;
	}

	template<typename T>
	void Entity::RemoveBehaviour()
	{
		static_assert(std::is_base_of_v<EntityBehaviour, T>,
			"Entity::RemoveBehaviour<T> requires T to derive from EntityBehaviour");

		if (!IsValid())
		{
			RADIANT_WARN("Entity::RemoveBehaviour: invalid entity handle - nothing detached");
			return;
		}

		// Silent when there is nothing to detach: the caller is asking for a state
		// that already holds, the same judgement DestroyEntity makes about an
		// already-condemned entity
		if (T* instance = GetBehaviour<T>())
			m_Level->DetachBehaviour(*instance);
	}

}
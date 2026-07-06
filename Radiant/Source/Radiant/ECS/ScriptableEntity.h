#pragma once

#include "Entity.h"

namespace Radiant {

	/**
	 * Base class for native C++ scripts — the engine's scripting seam. Subclass,
	 * override OnCreate/OnUpdate/OnDestroy, and attach with
	 * NativeScriptComponent::Bind<T>() (one script per entity; bindings are
	 * code-only and must be re-bound after level load).
	 *
	 * Lifecycle contract: the instance is heap-allocated lazily by the Level on
	 * the first Level::OnFixedUpdate after binding. m_Entity is wired AFTER
	 * construction, so constructors must not touch components — do first-time
	 * setup in OnCreate, which runs immediately after the entity is wired.
	 * OnUpdate then runs every FIXED simulation step (with the fixed delta, not
	 * a frame delta) while the entity's MetadataComponent is active (OnCreate
	 * and OnDestroy run regardless of the active flag).
	 * OnDestroy runs when the entity is destroyed; the instance is deleted by its
	 * owning NativeScriptComponent (side-table rework tracked as RAD-30).
	 */
	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() = default;

		// Component access forwards to the script's entity — valid from OnCreate
		// onward, never from the constructor (m_Entity is not yet wired there)
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		bool operator==(const ScriptableEntity& other) const
		{
			return m_Entity == other.m_Entity;
		}

		bool operator!=(const ScriptableEntity& other) const
		{
			return !(*this == other);
		}

		bool operator==(const Entity& other) const
		{
			return m_Entity == other;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	protected:
		// Defaults log INFO so an unbound override is visible — note the OnUpdate
		// default fires every frame for scripts that don't override it
		virtual void OnCreate() { RADIANT_INFO("Scriptable OnCreate"); }
		virtual void OnUpdate(Timestep ts) { RADIANT_INFO("Scriptable OnUpdate"); }
		virtual void OnDestroy() { RADIANT_INFO("Scriptable OnDestroy"); }

	private:
		Entity m_Entity;
		friend class Level;
	};

}
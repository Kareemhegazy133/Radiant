#pragma once

#include "Entity.h"
#include "PhysicsBody.h"

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

		/**
		 * This entity's physics handle — forces, impulses, velocities,
		 * kinematic targets (see PhysicsBody). Test it before use; it is false
		 * when the entity has no rigidbody.
		 *
		 * TEMPORARY, and RAD-95 REMOVES IT. Forwarded because a script cannot
		 * otherwise reach its own Entity (m_Entity is private), which would
		 * leave RAD-90's verbs unreachable from the only gameplay code the
		 * engine has. But one forwarder per subsystem is the wrong shape: GAS
		 * would want GetAbilitySystem(), animation GetAnimation(), and this
		 * class would grow a method per subsystem forever — the O(N)-edits-per-
		 * feature pattern RAD-94 exists to prevent. RAD-95 exposes the entity
		 * handle once, after which every facade comes free and this goes away.
		 * Do not add a second forwarder of this kind; extend RAD-95 instead.
		 *
		 * Called from OnUpdate, the effect lands in the step about to run;
		 * called from a collision hook, in the next one.
		 */
		PhysicsBody GetPhysicsBody() { return m_Entity.GetPhysicsBody(); }

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

		/**
		 * Called once when this entity's collider starts (Begin) or stops (End)
		 * touching another's, during the fixed step that detected it — after
		 * physics has advanced, so transforms are current. Silent by default,
		 * unlike the hooks above: a level of falling crates would log every
		 * landing.
		 *
		 * `other` MAY BE INVALID — check it before use. An invalid partner
		 * means it was destroyed before the notification could be delivered;
		 * for OnCollisionEnd that is the normal way "the thing I was standing
		 * on was deleted" arrives, one step after the deletion.
		 *
		 * The physics step is over by the time this runs, so anything is legal
		 * here: destroy entities (including `other`), teleport, spawn, add or
		 * remove components. One caveat — destroying THIS entity deletes the
		 * script instance whose method is executing, so it must be the last
		 * statement.
		 *
		 * Requires the collider to opt in (BoxCollider2DComponent::
		 * EnableContactEvents, on by default; Box2D reports the contact if
		 * EITHER shape opted in), and is skipped while the entity's
		 * MetadataComponent is inactive — the same gate OnUpdate uses.
		 */
		virtual void OnCollisionBegin(Entity other) {}
		virtual void OnCollisionEnd(Entity other) {}

	private:
		Entity m_Entity;
		friend class Level;
	};

}
#pragma once

#include "Entity.h"

// OnUpdate/OnCollision* name Timestep. Previously this header only ever
// compiled inside Level.cpp, which happened to pull it in first; giving
// ScriptableEntity its own translation unit made the omission visible.
#include "Radiant/Core/Timestep.h"

namespace Radiant {

	/**
	 * Base class for native C++ scripts — the engine's scripting seam. Subclass,
	 * override OnCreate/OnUpdate/OnDestroy, and attach with
	 * NativeScriptComponent::Bind<T>() (one script per entity; bindings are
	 * code-only and must be re-bound after level load).
	 *
	 * NAME IS PROVISIONAL: RAD-99 renames this class once RAD-101 settles
	 * whether an entity may carry one behaviour or several — the answer decides
	 * whether the right name is Actor (the behaviour IS the world-thing) or
	 * Behaviour (it is one of many attached to it).
	 *
	 * WHAT A SCRIPT CAN REACH (RAD-95). Two accessors, and everything chains
	 * off them:
	 *
	 *     GetEntity()                                    // my handle
	 *     GetEntity().GetComponent<TransformComponent>() // my components
	 *     GetEntity().GetPhysicsBody().ApplyForce(...)   // my physics
	 *     GetLevel().CreateEntity("Bullet")              // the world
	 *
	 * There are deliberately NO component or subsystem forwarders on this
	 * class. It previously mirrored four of Entity's methods, chosen by nobody
	 * and never revisited; every future subsystem facade is now reachable the
	 * day it is written, at a cost of zero edits here. See GetLevel() for the
	 * rule that keeps it that way.
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
	 *
	 * Scripts may spawn and destroy entities from any hook. Structural changes
	 * take effect immediately, but the script set is snapshotted at the start
	 * of each step — see Level::OnFixedUpdate's script-pass contract for what
	 * that guarantees.
	 */
	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() = default;

		/**
		 * The entity this script drives. Everything a script can reach hangs
		 * off it: components, physics (GetPhysicsBody), and every subsystem
		 * facade added later — none of which costs this class an edit.
		 *
		 * Valid from OnCreate onward, never in the constructor: the Level wires
		 * the handle AFTER construction.
		 */
		Entity GetEntity() const { return m_Entity; }

		/**
		 * The level this script's entity lives in, as gameplay may use it —
		 * spawn, find and destroy entities, and the level-wide collision
		 * channel. See GameplayLevel for what it deliberately withholds.
		 *
		 * WHY THERE ARE EXACTLY TWO ACCESSORS HERE, AND WHY THAT IS NOT THE
		 * FORWARDER PATTERN RETURNING (RAD-95). These two are *relationship
		 * navigation* — a bounded set, sized by the relationships a script
		 * actually has, and a script has two: the entity it drives and the
		 * level that entity is in. It grows only if a genuinely new
		 * relationship appears, which RAD-94 forbids fabricating. What must
		 * never be added is a *subsystem* forwarder (GetPhysicsBody,
		 * GetAbilitySystem, GetAnimation): that set is unbounded — one per
		 * subsystem, forever — which is why the RAD-90 stopgap was removed
		 * rather than joined. Reach subsystems through GetEntity().
		 *
		 * Unreal draws the same line: UActorComponent carries both GetOwner()
		 * and GetWorld(), and no per-subsystem forwarder.
		 */
		GameplayLevel GetLevel() const;

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
		/**
		 * Defaults log INFO so an unbound override is visible — note the
		 * OnUpdate default fires every step for scripts that don't override it.
		 *
		 * Destroying THIS entity from OnCreate or OnUpdate deletes the script
		 * instance whose method is executing, so it must be the last statement
		 * (the same caveat OnCollisionBegin carries). RAD-97 removes this wart
		 * rather than rewording it: once destruction is deferred to a reap
		 * point, the method finishes normally.
		 */
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
		 * statement (a wart RAD-97 removes by deferring the reap).
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
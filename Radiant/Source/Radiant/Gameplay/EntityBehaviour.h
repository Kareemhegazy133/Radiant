#pragma once

#include "Radiant/ECS/Entity.h"

// OnUpdate/OnCollision* name Timestep. Previously this header only ever
// compiled inside Level.cpp, which happened to pull it in first; giving
// EntityBehaviour its own translation unit made the omission visible.
#include "Radiant/Core/Timestep.h"

namespace Radiant {

	/**
	 * One unit of C++ gameplay behaviour attached to one entity — the root of
	 * the gameplay hierarchy every game type subclasses. Subclass, override
	 * OnCreate/OnUpdate/OnDestroy, and attach with
	 * NativeScriptComponent::Bind<T>() (one behaviour per entity today —
	 * RAD-101 makes it several; bindings are code-only and must be re-bound
	 * after level load).
	 *
	 * WHAT THIS CLASS IS, IN ONE SENTENCE (RAD-99). Entity is Radiant's
	 * world-thing and this is Radiant's behaviour unit — the same split Unreal
	 * draws between AActor and UActorComponent. The entity exists whether or
	 * not any behaviour is attached, and most entities have none, which is
	 * precisely why this class is not called Actor: a type that is optional on
	 * the things it would claim to be cannot be those things.
	 *
	 * WHAT A BEHAVIOUR CAN REACH. Two accessors, and everything chains off
	 * them:
	 *
	 *     GetOwner()                                    // my entity
	 *     GetOwner().GetLocation()                      // named gameplay verbs
	 *     GetOwner().GetComponent<SpriteComponent>()    // the escape hatch
	 *     GetOwner().GetPhysicsBody().ApplyForce(...)   // my physics
	 *     GetLevel().CreateEntity("Bullet")             // the world
	 *
	 * WHAT MAY EVER BE ADDED HERE — the inclusion rule, so this surface stays
	 * designed instead of accumulating. Exactly two kinds of member:
	 *
	 *   1. A LIFECYCLE HOOK — a method the engine calls on you at a defined
	 *      point in the frame. The five below are the current set.
	 *   2. A RELATIONSHIP ACCESSOR — navigation to something this behaviour
	 *      genuinely has a relationship with. That set is BOUNDED, and a
	 *      behaviour has two: the entity it drives and the level that entity is
	 *      in. A third requires a third *relationship* to exist first, which
	 *      RAD-94 forbids fabricating.
	 *
	 * And two kinds that must NEVER be added:
	 *
	 *   - AN ENTITY-SCOPE VERB (GetLocation, Teleport, Destroy). Those live on
	 *     Entity, per the dividing rule in playbook §10: a verb that names one
	 *     entity lives on Entity; a verb about the level as a whole lives on
	 *     GameplayLevel. Once an entity may carry SEVERAL behaviours (RAD-101),
	 *     an unqualified SetLocation() here would also be claiming to be "the"
	 *     entity while being one of many — and would duplicate the entity's
	 *     whole verb surface once per attached behaviour.
	 *   - A SUBSYSTEM FACADE (GetPhysicsBody, a future GetAbilitySystem,
	 *     GetAnimation). That set is UNBOUNDED — one per subsystem, forever —
	 *     which is the O(N)-edits-per-feature pattern RAD-94 exists to forbid,
	 *     and is why the RAD-90 stopgap was removed rather than joined. Reach
	 *     subsystems through GetOwner().
	 *
	 * Unreal draws both lines identically: UActorComponent carries GetOwner()
	 * and GetWorld() and nothing else relational, has no transform verbs at all
	 * (ActorComponent.h contains zero occurrences of "Location"), and no
	 * per-subsystem forwarder.
	 *
	 * HOW ANYONE FINDS A CONCRETE BEHAVIOUR (RAD-100). The engine stores every
	 * instance as an EntityBehaviour*, so recovering the subclass is a query on
	 * the entity: Entity::GetBehaviour<T>(), returning nullptr when the answer
	 * is "not that". Unreal's equivalent is AActor::FindComponentByClass<T>(),
	 * which returns null the same way; ours reaches the answer with
	 * dynamic_cast where UE uses its reflection system's IsA, because RAD-72 is
	 * not built yet.
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
	 * Behaviours may spawn and destroy entities from any hook. Structural
	 * changes take effect immediately, but the behaviour set is snapshotted at
	 * the start of each step — see Level::OnFixedUpdate's script-pass contract
	 * for what that guarantees.
	 */
	class EntityBehaviour
	{
	public:
		/**
		 * LOAD-BEARING TWICE, so do not "tidy it away" if the virtual hooks
		 * below ever move. It makes deletion through this base correct — the
		 * owning NativeScriptComponent deletes an EntityBehaviour* that really
		 * points at a subclass — AND it is what makes this class polymorphic,
		 * which is the precondition for the RTTI that Entity::GetBehaviour<T>()
		 * reads. Remove it and typed retrieval stops compiling somewhere far
		 * from here.
		 */
		virtual ~EntityBehaviour() = default;

		/**
		 * The entity this behaviour is attached to. Everything a behaviour can
		 * reach hangs off it: named gameplay verbs (GetLocation/SetLocation),
		 * components, physics (GetPhysicsBody), and every subsystem facade
		 * added later — none of which costs this class an edit.
		 *
		 * Named "owner" because the entity genuinely owns this instance: the
		 * entity's NativeScriptComponent holds it by an owning pointer and
		 * deletes it. The name is therefore true in Radiant's own ownership
		 * vocabulary (playbook §2) as well as matching UE's
		 * UActorComponent::GetOwner().
		 *
		 * Valid from OnCreate onward, never in the constructor: the Level wires
		 * the handle AFTER construction.
		 */
		Entity GetOwner() const { return m_Entity; }

		/**
		 * The level this behaviour's entity lives in, as gameplay may use it —
		 * spawn, find and destroy entities, and the level-wide collision
		 * channel. See GameplayLevel for what it deliberately withholds.
		 *
		 * The second and last relationship accessor. See the class doc above
		 * for why that set is bounded at two, and why these two are not the
		 * forwarder pattern returning.
		 */
		GameplayLevel GetLevel() const;

		bool operator==(const EntityBehaviour& other) const
		{
			return m_Entity == other.m_Entity;
		}

		bool operator!=(const EntityBehaviour& other) const
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
		 * Silent by default, like the collision hooks below. A behaviour that
		 * overrides nothing is a legal no-op — a marker, or a future Pawn base
		 * whose subclasses fill it in — not a mistake worth reporting; and the
		 * OnUpdate default in particular used to log every fixed step, which is
		 * 60 lines a second per behaviour.
		 *
		 * DESTROYING ANY ENTITY IS SAFE FROM HERE, including this one, and needs
		 * no positional discipline — `GetOwner().Destroy();` may be followed by
		 * more statements and the method runs to completion (RAD-97). Destruction
		 * marks the entity dead and defers the free to the end of the fixed step,
		 * so the instance outlives every method that was running when it was
		 * called. OnDestroy fires immediately, before this method resumes.
		 *
		 * What DOES change immediately: the entity reports IsValid() == false
		 * from the moment you destroy it, so anything after that line reads a
		 * dead handle. That is a reason to put the destroy last out of taste, not
		 * out of safety.
		 */
		virtual void OnCreate() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnDestroy() {}

		/**
		 * Called once when this entity's collider starts (Begin) or stops (End)
		 * touching another's, during the fixed step that detected it — after
		 * physics has advanced, so transforms are current.
		 *
		 * `other` MAY BE INVALID — check it before use. An invalid partner
		 * means it was destroyed before the notification could be delivered;
		 * for OnCollisionEnd that is the normal way "the thing I was standing
		 * on was deleted" arrives, one step after the deletion.
		 *
		 * The physics step is over by the time this runs, so anything is legal
		 * here: destroy entities (including `other` and this one), teleport,
		 * spawn, add or remove components. No positional discipline is required
		 * — destruction marks and defers the free to the end of the step, so the
		 * instance outlives the call and the method runs to completion (RAD-97).
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
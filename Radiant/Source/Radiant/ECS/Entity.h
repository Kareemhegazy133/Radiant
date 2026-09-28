#pragma once

#include <entt.hpp>

#include "Components.h"

#include "Radiant/Physics/TeleportType.h"

#include <vector>

namespace Radiant {

	class Level;
	// Both are returned by value from accessors below, which needs only an
	// incomplete type to DECLARE — the definitions live in Entity.cpp, which
	// includes their headers. Each of those headers includes this one (
	// PhysicsBody holds an Entity by value; GameplayLevel reaches Level, which
	// includes this header), so declaring either the other way round would be
	// a cycle.
	class PhysicsBody;
	class GameplayLevel;

	/**
	 * Value handle to an entity: {entt handle, Level*}, 16 bytes, copied freely.
	 * Owns nothing — the Level owns all entity and component storage.
	 *
	 * Handles are TRANSIENT: they dangle once the entity (or its Level) is
	 * destroyed, and are never serialized — the persistent identity is the UUID
	 * (GetUUID / Level::GetEntityByUUID). Check IsValid() / operator bool before
	 * using a handle you did not just obtain from the Level. Main-thread only.
	 *
	 * Component accessors assert on misuse in Debug/Release (asserts compile out
	 * in Dist): GetComponent on a missing component, AddComponent on a duplicate,
	 * and every accessor on an invalid handle.
	 *
	 * THE NAMED GAMEPLAY VERBS, AND THE RULE FOR ADDING ONE (RAD-99).
	 * Entity is Radiant's world-thing — Unreal's AActor to EntityBehaviour's
	 * UActorComponent — so a verb that names ONE entity lives here (playbook
	 * §10's dividing rule; a verb about the level as a whole lives on
	 * GameplayLevel). GetLocation/SetLocation/GetRotation/SetRotation and the
	 * paired SetTransform are that surface today.
	 *
	 * SetTransform is worth reading as the rule working rather than as an
	 * exception to it. The set was designed as four verbs; the FIRST real
	 * customer — Reaper's CameraController, which sets both halves every fixed
	 * step — showed that calling the pair costs roughly four transform lookups
	 * where one would do, in exactly the position a redundant double lookup
	 * once hid for a year (playbook §10). It passes the semantic gate like its
	 * siblings and passes the customer gate on that evidence, so it was
	 * admitted BY the rule below rather than argued in around it.
	 *
	 * A verb is admitted only when BOTH gates pass:
	 *
	 *   SEMANTIC GATE — it names exactly one entity, is expressible without the
	 *     caller naming a component type, and EITHER (a) the equivalent raw
	 *     component write is *silently wrong*, OR (b) it is the read half of
	 *     such a write.
	 *   CUSTOMER GATE — something in the engine or the game needs it now
	 *     (RAD-94: adopt when the need exists, never before).
	 *
	 * That bounds this surface by the number of silent-failure traps in the
	 * engine — small, and shrinking — rather than by the number of components,
	 * which grows forever. If it ever passes roughly a dozen verbs, the rule has
	 * stopped working and the answer is a general mechanism, not a thirteenth
	 * verb.
	 *
	 * Deliberately EXCLUDED, so the next addition meets a decision rather than a
	 * precedent:
	 *   - SetScale / GetScale — passes the semantic gate (a scale change leaves
	 *     the physics shape stale until RefreshCollider) but fails the customer
	 *     gate: nothing scales anything yet. Admit them by rule the day
	 *     something does.
	 *   - SetActive / IsActive — fails the semantic gate. Writing
	 *     MetadataComponent::IsActive directly is correct, so a verb would be a
	 *     rename, not a fix.
	 *   - SetColor, SetTexture, and every other "read/write this field" —
	 *     fails the semantic gate. GetComponent<T>() is right there, and is the
	 *     escape hatch by design.
	 *   - GetVelocity and the rest of the physics surface — those are SUBSYSTEM
	 *     state and live on the PhysicsBody facade (GetPhysicsBody()), not here.
	 *     Hoisting them up would restart the unbounded one-accessor-per-
	 *     subsystem growth RAD-94 exists to prevent.
	 *
	 * Note this does NOT reopen RAD-94's rejection of typed component accessors
	 * (entity.GetSprite()). That rejection is about handing back a COMPONENT,
	 * which would make Entity.h grow per component type. GetLocation returns a
	 * glm::vec3 and hides which component stores it: a verb, not an accessor.
	 *
	 * THE GATES GOVERN NAMED VERBS ONLY (RAD-100). They do not apply to the
	 * generic template queries — GetComponent/TryGetComponent/HasComponent, and
	 * GetBehaviour below. Those are parameterised by type, so each costs this
	 * header exactly one declaration however many component or behaviour types
	 * exist: they are bounded by CONSTRUCTION, where the gates bound by
	 * discipline. Filing one under the gates would mean widening a rule that
	 * nothing needed widened, and a vaguer rule is a weaker one. A new member
	 * belongs to whichever family it is: templated over the type it returns, or
	 * named for a specific thing it does.
	 */
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Level* level)
			: m_EntityHandle(handle), m_Level(level) {}
		Entity(const Entity& other) = default;

		~Entity() = default;

		/**
		 * Constructs T in place from args and returns a reference into the component
		 * pool — invalidated by later pool mutations, so do not cache it. Asserts if
		 * the entity already has T. Adding a physics component fires its entt
		 * construct signal (creates the Box2D body/fixture) before returning.
		 */
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		/** As AddComponent, but replaces an existing T instead of asserting. */
		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args);

		/**
		 * Returns a reference to the entity's T; asserts if the component is missing.
		 * The reference is invalidated by later pool mutations — do not cache it.
		 */
		template<typename T>
		T& GetComponent();

		template<typename T>
		const T& GetComponent() const;

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		T* TryGetComponent();

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		const T* TryGetComponent() const;

		/** True if the entity has all of the listed component types. */
		template<typename... T>
		bool HasComponent();

		template<typename... T>
		bool HasComponent() const;

		/**
		 * Removes T immediately; asserts if missing. Removal fires the component's
		 * entt on_destroy signal (for physics components this destroys the body).
		 */
		template<typename T>
		void RemoveComponent();

		/** As RemoveComponent, but a no-op when the component is absent. */
		template<typename T>
		void RemoveComponentIfExists();

		/**
		 * The behaviour of type T driving this entity, or nullptr — the checked
		 * way to recover a concrete EntityBehaviour subclass from a handle
		 * (RAD-100). Gameplay names neither the component the instance lives in
		 * nor a cast the compiler cannot verify:
		 *
		 *     if (Door* door = other.GetBehaviour<Door>())
		 *         door->Open();
		 *
		 * A QUERY, SO IT NEVER ASSERTS. "It isn't that" is an answer, not a
		 * programmer error, so all four ways to get nothing back are silent:
		 *
		 *   1. this handle is invalid — the DOCUMENTED NORMAL case, not a
		 *      mistake: a collision partner may already be dead by the time you
		 *      ask (EntityBehaviour::OnCollisionEnd's contract), so a warning
		 *      here would fire during correct gameplay;
		 *   2. the entity has no behaviour at all, which is true of most entities;
		 *   3. it has behaviours, but none of them is a T;
		 *   4. the matching behaviour has been detached (RemoveBehaviour<T>) and
		 *      is awaiting the reap — its OnDestroy has already run, so handing
		 *      it back would be the same ordering bug as calling OnUpdate after
		 *      OnDestroy.
		 *
		 * A FIFTH CASE RETIRED with RAD-101's eager construction: "it has one but
		 * the instance is not built yet" can no longer occur, because
		 * AddBehaviour<T> constructs at the call site instead of leaving a factory
		 * for the Level to invoke on the next step. Documenting a state that
		 * cannot happen teaches a reader to distrust the rest of the block.
		 *
		 * The only check that fires is compile-time: T must derive from
		 * EntityBehaviour.
		 *
		 * THE POINTER IS TRANSIENT, exactly like the handle it came from. It
		 * dangles when the entity dies, when the Level dies, and when the
		 * behaviour is detached — the reap deletes the instance the old pointer
		 * names. Never store one across a fixed step: keep the Entity (or its
		 * UUID) and ask again, which is cheap enough to do per use. Now that an
		 * entity may carry SEVERAL behaviours (RAD-101), a SIBLING's actions can
		 * invalidate a stored pointer too — it is no longer only your own entity
		 * dying — so this contract is load-bearing rather than cautionary.
		 *
		 * const here constrains this handle, not the behaviour it names — the
		 * same latitude GetPhysicsBody and GetLevel take, and for the same
		 * reason: a handle is a pointer, and const on a pointer is not const on
		 * its pointee.
		 *
		 * Asking for the base type is legal and deliberate:
		 * GetBehaviour<EntityBehaviour>() returns whatever is bound, because
		 * the underlying question is "is it a T, or derived from T".
		 */
		template<typename T>
		T* GetBehaviour() const;

		/**
		 * Constructs a behaviour of type T, attaches it to this entity, and
		 * returns it so the caller can configure it immediately:
		 *
		 *     entity.AddBehaviour<Health>()->MaxHP = 50.f;
		 *
		 * CONSTRUCTION IS EAGER; OnCreate IS NOT. The object exists the moment
		 * this returns — that is the point of returning a usable pointer — but
		 * OnCreate is gameplay (it may spawn, destroy, or attach more) and so
		 * still runs at the next script pass, at the one defined point. Firing it
		 * from an arbitrary call site is exactly the re-entrancy RAD-95 and RAD-97
		 * spent two cards confining.
		 *
		 * DUPLICATE TYPES ARE REJECTED (RAD-101 D5), which is where we diverge
		 * from UE deliberately: its components are things (two meshes is a car
		 * with two doors), ours are logic units, and refusing duplicates is what
		 * makes GetBehaviour<T> a total question — "the T" stays a true phrase
		 * instead of degrading to "whichever we hit first". On a duplicate this
		 * asserts, warns, and returns the EXISTING instance, so Dist recovers
		 * rather than running with two of something documented as singular.
		 *
		 * A VERB, so an unusable handle WARNS — unlike GetBehaviour, which
		 * answers null in silence. The caller asked for an action on something it
		 * believed existed (playbook §10).
		 *
		 * The returned pointer is transient on exactly GetBehaviour's terms: the
		 * Level owns the instance and frees it at the reap. Do not store it.
		 */
		template<typename T, typename... Args>
		T* AddBehaviour(Args&&... args);

		/** True if a behaviour of type T is attached. Exactly GetBehaviour<T>() != nullptr. */
		template<typename T>
		bool HasBehaviour() const;

		/**
		 * Detaches the behaviour of type T, running its OnDestroy if one is owed.
		 *
		 * DEFERRED, like entity destruction and for the identical reason (D12):
		 * this MARKS, and the reap frees at the end of the fixed step. So a
		 * behaviour may detach ITSELF from inside its own OnUpdate — the common
		 * case, "I am done, take me off" — and its method still runs to
		 * completion. An immediate delete would make `this` dangle mid-call.
		 *
		 * From the mark onward the behaviour is gone to every observer:
		 * GetBehaviour<T> stops finding it and no further hook fires. Detaching
		 * something not attached is a silent no-op — a state that already holds.
		 */
		template<typename T>
		void RemoveBehaviour();

		/**
		 * Every behaviour attached to this entity, in attach order — UE's
		 * AActor::GetComponents(), and the one member of this family that is not
		 * parameterised by type.
		 *
		 *     for (EntityBehaviour* b : entity.GetBehaviours())
		 *         ...
		 *
		 * NON-OWNING: it yields raw observers, never the Level's Scopes — handing
		 * back the ownership handles would hand back exactly what the Level exists
		 * to keep (playbook §10). Detached-but-not-yet-reaped behaviours are
		 * skipped, so this agrees with GetBehaviour<T> instead of contradicting it.
		 *
		 * BY VALUE, AND THAT ALLOCATES. A deliberate reversal of this card's own
		 * earlier plan, which specified an allocation-free view: the view needs a
		 * filtering iterator, the filter must read a private flag, and reading it
		 * needs friendship EntityBehaviour does not grant — so the "cheap" option
		 * costs a friend declaration and an out-of-line iterator to save an
		 * allocation on a path nothing calls per frame. UE reaches for
		 * TInlineComponentArray because it collects components constantly at scale;
		 * we do not, and inventing that constraint before a caller exists is how
		 * machinery gets built for nobody. If a hot caller appears, its measurement
		 * is what justifies the view.
		 *
		 * THE POINTERS ARE TRANSIENT, exactly as GetBehaviour<T>'s are: a detach or
		 * the entity's death frees what they name. Do not hold them across a
		 * gameplay call. An invalid handle yields an empty vector rather than
		 * warning — this is a query, and "none" is an answer.
		 */
		std::vector<EntityBehaviour*> GetBehaviours() const;

		/**
		 * Where this entity is, in world units. One component lookup — the same
		 * one GetComponent<TransformComponent>().Translation performs, with the
		 * storage detail hidden. Returns {0,0,0} after a WARN on an invalid
		 * handle, because a getter has no lower layer to delegate the check to.
		 *
		 * For a body-backed entity this is the pose physics wrote back at the
		 * end of the last step, which is the current truth when gameplay runs.
		 */
		glm::vec3 GetLocation() const;

		/**
		 * Rotation about Z in RADIANS — the 2D rotation. The transform's X/Y
		 * Euler terms are authored data this surface does not speak about (the
		 * same choice Teleport makes). Returns 0 after a WARN on an invalid
		 * handle.
		 */
		float GetRotation() const;

		/**
		 * Moves this entity CONTINUOUSLY — the everyday "it moved" verb, where
		 * Teleport below is "it jumped". Forwarders to Level::SetLocation /
		 * Level::SetRotation (see them for the full contract: ECS write, the
		 * physics push that stops this being the silent no-op a raw transform
		 * write is on a body, and the preserved render snapshot that keeps
		 * motion interpolated). The only logic here is warning on a level-less
		 * handle, which cannot reach the Level to be warned by it.
		 *
		 * translation is world units; radians is about Z. Velocity is kept —
		 * reach for Teleport(ResetVelocity) to clear it. Never swept: no
		 * collisions occur along the way. Driving a DYNAMIC body with these
		 * every step fights the solver — use GetPhysicsBody()'s force and
		 * velocity verbs, or MoveKinematic for a kinematic body.
		 *
		 * PREFER SetTransform WHEN SETTING BOTH. SetLocation must read the
		 * current rotation to preserve it (and SetRotation the translation), so
		 * calling the pair costs roughly four transform lookups where
		 * SetTransform costs one. That is not a micro-optimisation here: these
		 * run every fixed step for anything that moves under its own power, and
		 * a redundant double lookup in exactly this position went unnoticed in
		 * Reaper's CameraController for a year (playbook §10).
		 */
		void SetTransform(const glm::vec3& translation, float radians);
		void SetLocation(const glm::vec3& translation);
		void SetRotation(float radians);

		/**
		 * Moves this entity discontinuously — forwarders to Level::Teleport
		 * (see it for the full contract: ECS write + snapshot reset + explicit
		 * physics push, and what teleportType does to velocity); the only
		 * logic here is warning on a level-less handle, which cannot reach the
		 * Level to be warned about. rotationZ is radians; the overload without
		 * it keeps the current rotation.
		 */
		void Teleport(const glm::vec3& translation, float rotationZ, TeleportType teleportType = TeleportType::KeepVelocity);
		void Teleport(const glm::vec3& translation, TeleportType teleportType = TeleportType::KeepVelocity);

		/**
		 * Destroys this entity — a forwarder to Level::DestroyEntity (see it for
		 * the full contract: OnDestroy, eager physics teardown, and the deferred
		 * reap). Exists so gameplay that only holds an Entity — a behaviour, a
		 * collision handler — can destroy without reaching for the Level.
		 *
		 * SAFE FROM ANYWHERE, including on the entity whose own hook is running,
		 * and including while a view is being iterated: nothing is freed in the
		 * call, so no iterator and no executing method loses what it stands on
		 * (RAD-97). It need not be the last statement.
		 *
		 * Every handle to this entity, including this one, reports IsValid() ==
		 * false from this point — the entity is dead immediately even though its
		 * storage is released at the end of the fixed step.
		 */
		void Destroy();

		/**
		 * Re-applies this entity's collider fields and transform Scale to its
		 * live physics shape — a forwarder to Level::RefreshCollider (see it
		 * for the full contract). Nothing detects collider or Scale writes
		 * implicitly, so gameplay that mutates them must call this or the body
		 * keeps its old shape. The only logic here is warning on a level-less
		 * handle, which cannot reach the Level to be warned by it.
		 */
		void RefreshCollider();

		// The non-const overload hands out a mutable reference to the shared static
		// fallback when metadata is missing — do not write through it in that case
		std::string& Name() { return HasComponent<MetadataComponent>() ? GetComponent<MetadataComponent>().Tag : NoName; }
		const std::string& Name() const { return HasComponent<MetadataComponent>() ? GetComponent<MetadataComponent>().Tag : NoName; }

		operator bool() const;
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		/**
		 * This entity's physics handle — how gameplay applies forces,
		 * impulses, velocities and kinematic targets (see PhysicsBody for the
		 * full contract and for why those verbs live there rather than here).
		 *
		 * Always returns a handle; test it before use. It is false when the
		 * entity has no rigidbody, no live Level, or a Level with no physics
		 * world, so `if (PhysicsBody body = entity.GetPhysicsBody())` is the
		 * idiom. Cheap enough to call per use — it copies this handle and
		 * nothing else — so prefer calling it again over storing one.
		 */
		PhysicsBody GetPhysicsBody() const;

		/**
		 * The level this entity lives in, as gameplay may use it — how a
		 * script spawns, finds and destroys entities, and subscribes to the
		 * level-wide collision channel (see GameplayLevel for what it
		 * deliberately withholds, and why).
		 *
		 * Always returns a handle; test it before use. It is false only when
		 * this handle has no level at all, so
		 * `if (GameplayLevel level = entity.GetLevel())` is the idiom. Cheap
		 * enough to call per use — it copies one pointer — so prefer calling
		 * it again over storing one.
		 *
		 * const returns a handle through which the level CAN be mutated, the
		 * same latitude GetPhysicsBody takes: const here constrains this
		 * handle, not the thing it names, exactly as with a pointer.
		 */
		GameplayLevel GetLevel() const;

		/** The entity's persistent identity — stable across save/load, unlike this handle. */
		UUID GetUUID() { return GetComponent<MetadataComponent>().ID; }

		/** True when this handle refers to a live entity in a live Level. */
		bool IsValid() const;

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Level == other.m_Level;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Level* m_Level = nullptr;

		inline static std::string NoName = "Unnamed";

		friend class Level;
		// Reads m_Level to route its verbs through Level::ResolvePhysics. The
		// alternative — storing the Level* in PhysicsBody as well — would keep
		// the same pointer in two places for the sake of avoiding one friend
		// declaration between two types in the same module.
		friend class PhysicsBody;
	};
}
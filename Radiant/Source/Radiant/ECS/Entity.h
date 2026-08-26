#pragma once

#include <entt.hpp>

#include "Components.h"

#include "Radiant/Physics/TeleportType.h"

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
		 *   2. the entity has no NativeScriptComponent — it has no behaviour at
		 *      all, which is true of most entities;
		 *   3. it has one, but the instance is not built yet — behaviours are
		 *      created lazily on the first fixed step after Bind<T>(), so this
		 *      is timing, not error;
		 *   4. the instance exists and is not a T.
		 *
		 * The only check that fires is compile-time: T must derive from
		 * EntityBehaviour.
		 *
		 * THE POINTER IS TRANSIENT, exactly like the handle it came from. It
		 * dangles when the entity dies, when the Level dies, and when the
		 * behaviour is rebound — AddOrReplaceComponent<NativeScriptComponent>
		 * deletes the instance the old pointer names. Never store one across a
		 * fixed step: keep the Entity (or its UUID) and ask again, which is
		 * cheap enough to do per use. Once an entity may carry SEVERAL
		 * behaviours (RAD-101) a sibling's actions can invalidate a stored
		 * pointer too, so this contract only gets more load-bearing.
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
		 * Destroys this entity immediately — a forwarder to Level::DestroyEntity
		 * (see it for the full contract: script OnDestroy, physics teardown, and
		 * the rule against calling it while iterating a view). Exists so gameplay
		 * that only holds an Entity — a native script, a collision handler — can
		 * destroy without reaching for the Level. Every handle to this entity,
		 * including this one, dangles afterwards.
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
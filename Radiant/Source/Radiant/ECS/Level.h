#pragma once

#include <entt.hpp>

#include "Radiant/Core/GameApplication.h"
#include "Radiant/Core/UUID.h"
#include "Radiant/Physics/ContactEvent.h"
#include "Radiant/Physics/TeleportType.h"

#include "Entity.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Radiant {

	class PhysicsWorld2D;

	/**
	 * The world: one set of entities plus the loops that update and render them.
	 *
	 * Owns the entt registry privately — all access goes through Level/Entity APIs;
	 * the registry is never exposed. A Level is a ref-counted Asset (held via
	 * Ref<Level>, loadable through the AssetManager). Main-thread only.
	 *
	 * LIFECYCLE CONTRACT. Creation is IMMEDIATE. Destruction is DEFERRED, in two
	 * halves (RAD-97):
	 *
	 *   MARK — DestroyEntity runs EVERY attached behaviour's OnDestroy (reverse
	 *     attach order — RAD-101), tears the physics body down EAGERLY, erases the
	 *     UUID map entry, and tags the entity PendingDestroyComponent. From that
	 *     instant the entity is dead to everything: IsValid() is false, lookups
	 *     miss it, and every gameplay pass excludes it. Safe to call from anywhere
	 *     — a collision handler, a script's OnUpdate, an entity on itself — with no
	 *     "last statement" discipline.
	 *   REAP — ReapDestroyedEntities frees every behaviour instance and the entt
	 *     row, at ONE defined point: the end of the fixed step, after contact
	 *     dispatch. Not end of frame, so a rendered frame never observes a
	 *     half-destroyed entity, and so the pending window stays exactly one step
	 *     regardless of how many steps a frame runs (playbook §1).
	 *
	 * The reap does two more jobs for the same reason it does this one — a walk may
	 * be holding an index into what they touch. It erases behaviours DETACHED from
	 * entities that are still alive (RemoveBehaviour<T> only marks, exactly as
	 * DestroyEntity does), and it reconciles the behaviour walk order in one pass.
	 * So it runs on every step, not only on steps where something died.
	 *
	 * Physics dies at the mark and not at the reap deliberately: a body surviving
	 * to the reap would keep generating contacts for the rest of the step, so a
	 * destroyed bullet would hit twice.
	 *
	 * The consequence worth planning around: an entity marked OUTSIDE a fixed
	 * step — a cheat key, a UI action, a destroy while paused — is not reaped
	 * until the next step, and never if the level is never stepped again. It is
	 * invalid and invisible throughout, so the cost is memory rather than
	 * behaviour, and ~Level collects it.
	 *
	 * Physics lifecycle is driven by the entt signals registered in the
	 * constructor: adding a RigidBody2DComponent creates the Box2D body and
	 * removing it destroys the body; the same holds for BoxCollider2DComponent
	 * and its shape — component presence IS the physics binding.
	 *
	 * The owner (Reaper's GameLayer) drives OnFixedUpdate at the engine's fixed
	 * simulation rate and OnRender once per rendered frame.
	 * Each Level owns its physics world (Scope<PhysicsWorld2D>) — worlds are
	 * independent simulations, so any number of Levels may coexist (RAD-27).
	 */
	class Level : public Asset
	{
	public:
		/**
		 * A contact resolved to entities — the gameplay-facing twin of the
		 * physics module's ContactEvent. EITHER handle may be invalid, meaning
		 * that side was destroyed before the notification was delivered; never
		 * both, since an event with nobody left to tell is skipped. The handles
		 * are valid for the duration of the call only — resolve by UUID if you
		 * need to remember a participant.
		 */
		struct CollisionEvent
		{
			Entity A;
			Entity B;
			ContactPhase Phase;
		};

		/**
		 * Opaque, copyable identity for a registered collision callback. Index
		 * says which slot, Generation says which lifetime of that slot — so a
		 * handle to a removed callback is always safe, even after its slot has
		 * been reused: RemoveCollisionCallback on it is a no-op instead of
		 * unregistering whoever inherited the slot. Same shape and reasoning as
		 * TimerHandle (playbook §2).
		 */
		struct CollisionCallbackHandle
		{
			static constexpr uint32_t InvalidIndex = 0xFFFFFFFF;

			uint32_t Index = InvalidIndex;
			uint32_t Generation = 0;
		};

		/**
		 * Constructs an empty level. Constructing with initialize == true creates
		 * this level's own physics world and connects the physics component
		 * signals. Pass initialize == false only for scratch levels that merely
		 * stage entities for (de)serialization — they get no world and no
		 * signals, and their destruction touches no other Level's physics.
		 */
		Level(const std::string& name = "UntitledLevel", bool initialize = true);

		// Defined out of line in Level.cpp, and that is REQUIRED rather than
		// stylistic: m_PhysicsWorld and m_Behaviours both hold Scopes to
		// forward-declared types, and unique_ptr's deleter needs the complete
		// type wherever the destructor is compiled. Writing `= default` here
		// breaks both with an incomplete-type error that names neither member.
		~Level();

		/**
		 * Creates a live entity immediately, with a fresh UUID, a TransformComponent,
		 * and a MetadataComponent tagged with name ("Entity" if empty).
		 */
		Entity CreateEntity(const std::string& name = std::string());

		/**
		 * As CreateEntity, but with a caller-supplied UUID (deserialization path).
		 * The UUID must be unique within this level — a duplicate silently replaces
		 * the previous entity's entry in the UUID lookup map.
		 */
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());

		/**
		 * MARKS the entity destroyed — see the lifecycle contract above for the
		 * mark/reap split. Runs the behaviour's OnDestroy (if an instance
		 * exists), removes the signal-bound physics components first so their
		 * on_destroy handlers can still read Metadata/Transform, erases the UUID
		 * map entry, and tags the entity. The entt row and the behaviour
		 * instance are freed at the reap, at the end of this fixed step.
		 *
		 * SAFE FROM ANYWHERE, including while iterating: nothing is freed here,
		 * so no view is invalidated and no running method loses the object it
		 * belongs to. Destroying the entity whose hook is executing is legal and
		 * need NOT be the last statement.
		 *
		 * Afterwards every handle to this entity, including the caller's, reports
		 * IsValid() == false. Destroying an already-marked entity is a silent
		 * no-op — the state it asks for already holds, and two systems
		 * independently deciding the same thing is dead in one step is normal
		 * gameplay (UE takes the same early-out in UWorld::DestroyActor). An
		 * invalid handle warns and recovers.
		 */
		void DestroyEntity(Entity entity);

		/**
		 * UUID overload of DestroyEntity; a no-op for unknown UUIDs — which
		 * includes already-marked entities, since the mark erases the map entry.
		 */
		void DestroyEntity(UUID entityID);

		/**
		 * Moves an entity discontinuously — the ONLY way to reposition an
		 * entity with a body (physics owns dynamic transforms; direct writes
		 * to TransformComponent have no physical effect — RAD-28). Works on
		 * any entity: writes the ECS transform, resets the render snapshot so
		 * the jump doesn't smear across a frame, and, if the entity has a
		 * body, pushes the pose into Box2D (body woken).
		 * rotationZ is radians; the overload without it keeps the current
		 * rotation. teleportType decides what the body's velocity does on
		 * arrival and defaults to KeepVelocity — portal semantics, and what
		 * this verb has always done; pass ResetVelocity for respawn semantics
		 * (see TeleportType). Body-less entities ignore it: there is no
		 * velocity to keep or reset. Invalid handles warn and recover.
		 */
		void Teleport(Entity entity, const glm::vec3& translation, float rotationZ, TeleportType teleportType = TeleportType::KeepVelocity);
		void Teleport(Entity entity, const glm::vec3& translation, TeleportType teleportType = TeleportType::KeepVelocity);

		/**
		 * Moves an entity CONTINUOUSLY — the everyday "it moved" verb, where
		 * Teleport above is "it jumped". Same two halves as Teleport (ECS
		 * transform write, plus a push into Box2D for body-backed entities, so
		 * this is never the silent no-op a raw TransformComponent write is on a
		 * body — RAD-28), and two deliberate differences:
		 *
		 *   - The render snapshot is PRESERVED, so motion stays interpolated
		 *     between simulation steps. Teleport stamps it instead, which is
		 *     what stops a jump smearing across a frame but would pin
		 *     continuous motion to fixed-step granularity.
		 *   - Nothing is logged. Teleport TRACEs because it is a rare,
		 *     deliberate act; a follow-camera calling this every step would
		 *     emit 60 lines a second (playbook §4).
		 *
		 * translation is world units; rotationZ is RADIANS about Z, and the
		 * transform's X/Y Euler terms are left untouched — the same choice
		 * Teleport makes. Velocity is always kept: a continuous move has no
		 * reason to discard it, and a caller who wants it cleared wants
		 * Teleport(ResetVelocity). Never a swept move — no collisions occur
		 * along the way (Radiant has no query API until RAD-76, where UE's
		 * SetActorLocation offers bSweep). Moving a DYNAMIC body this way every
		 * step fights the solver, which will correct it: continuous movement of
		 * a dynamic body belongs to PhysicsBody's force and velocity verbs, and
		 * of a kinematic body to MoveKinematic. Invalid handles warn and
		 * recover.
		 *
		 * The overloads keep the half you do not name: SetLocation keeps the
		 * current rotation, SetRotation keeps the current translation.
		 */
		void SetTransform(Entity entity, const glm::vec3& translation, float rotationZ);
		void SetLocation(Entity entity, const glm::vec3& translation);
		void SetRotation(Entity entity, float rotationZ);

		/**
		 * Re-applies an entity's BoxCollider2DComponent fields and transform
		 * Scale to its live Box2D shape, in place — call after mutating
		 * collider properties or Scale; nothing detects those writes
		 * implicitly (RAD-28). Asserts if the entity has no collider
		 * component; a collider without a live shape warns and recovers.
		 */
		void RefreshCollider(Entity entity);

		/**
		 * Registers a callback invoked once per collision with BOTH
		 * participants — the channel for level-wide systems (damage, audio,
		 * VFX, abilities) that belong to no single entity's script. These run
		 * BEFORE per-entity script hooks, so they see the collision before
		 * gameplay starts reacting to it. An empty callable is a programmer
		 * error (asserted; returns an invalid handle).
		 *
		 * CALLBACK LIFETIME CONTRACT: the Level owns the callback by value, and
		 * therefore owns whatever it captured. A callback capturing an object
		 * (an Entity, a system pointer, `this`) outlives its target unless the
		 * owner removes the handle in its teardown path — RemoveCollisionCallback
		 * releases the callback and its captures immediately. The Level cannot
		 * detect a subscriber that died without unregistering.
		 *
		 * Registering from inside a collision callback is legal: the new
		 * callback starts receiving events with the NEXT batch, never the one
		 * being dispatched.
		 */
		CollisionCallbackHandle AddCollisionCallback(std::function<void(const CollisionEvent&)> callback);

		/**
		 * Unregisters the callback, releases it (and its captures), and
		 * resets the handle. Stale, invalid, and already-removed handles are
		 * benign no-ops by design — this is the correct idiom for "remove if
		 * still registered". Safe to call from inside a collision callback,
		 * including on itself: the release is deferred to the end of the batch,
		 * since freeing a std::function that is currently executing would
		 * destroy the running lambda's own captures.
		 */
		void RemoveCollisionCallback(CollisionCallbackHandle& handle);

		/**
		 * Advances the simulation one FIXED step: runs native scripts (lazily
		 * instantiating unconstructed instances — see EntityBehaviour), steps
		 * the physics world, then drains the world's move events — only the
		 * bodies that actually moved — back into ECS transforms as a dedicated
		 * post-step pass. Physics owns the transform of
		 * dynamic bodies — nothing here pushes ECS transforms into Box2D
		 * (RAD-28); pushes happen only at spawn and through the explicit
		 * verbs. Call with the engine's fixed delta (from Layer::OnFixedUpdate)
		 * — never a variable frame delta.
		 *
		 * SCRIPT-PASS CONTRACT (RAD-95). Scripts may create and destroy
		 * entities, and those changes take effect IMMEDIATELY — but the set of
		 * scripts to run is snapshotted before the first one executes, so:
		 * an entity spawned during a step first receives its own OnCreate and
		 * OnUpdate on the NEXT step, and an entity destroyed during a step is
		 * skipped for the remainder of this one. Both are guarantees gameplay
		 * may rely on, not implementation details. A spawned entity also has
		 * no render snapshot for the frame it was born in (the snapshot pass
		 * runs before scripts), so it draws un-interpolated once — correct,
		 * since there is no previous pose to blend from.
		 */
		void OnFixedUpdate(Timestep ts);

		/**
		 * Renders sprite entities through Renderer2D using the first CameraComponent
		 * found with Primary set (iteration order — with several Primary cameras the
		 * pick is effectively arbitrary; with none, nothing renders). alpha in [0,1]
		 * (Time::GetAlpha()) blends movable entities between the last two simulation
		 * states: entities with a TransformSnapshotComponent draw
		 * lerp(snapshot, current, alpha); everything else draws current. Read-only
		 * with respect to simulation state (playbook §1): physics readback happens
		 * in OnFixedUpdate's post-step pass, not here.
		 */
		void OnRender(float alpha);

		/**
		 * Propagates the new viewport size to every camera that is not
		 * FixedAspectRatio. No-op when the size is unchanged.
		 */
		void OnViewportResize(uint32_t width, uint32_t height);

		/**
		 * Linear scan over all entities; returns the first whose tag matches, or an
		 * invalid Entity when none does. O(n) — do not call per frame in hot paths.
		 */
		Entity FindEntityByName(std::string_view name);

		/**
		 * Resolves a persistent UUID to a live entity; returns an invalid Entity for
		 * unknown UUIDs. UUIDs are the durable identity — entt handles are transient
		 * and never serialized.
		 */
		Entity GetEntityByUUID(UUID uuid);

		/** The level's asset handle doubles as its persistent identity. */
		UUID GetUUID() const { return Handle; }

		/**
		 * Collects every asset handle referenced by this level's components (sprite
		 * textures, fonts). Handles that fail AssetManager validation are excluded.
		 */
		std::unordered_set<AssetHandle> GetAssetList();

		void SetName(const std::string& name) { m_Name = name; }
		const std::string& GetName() const { return m_Name; }

	protected:
		/**
		 * THE DEFAULT VIEW for every pass gameplay or rendering can observe:
		 * entities matching Components... that are not pending destruction.
		 *
		 * The exclusion lives here rather than at each call site on purpose
		 * (playbook §4, one guard per layer, applied to iteration). Spelling it
		 * out per view would be an O(N)-edits-per-feature rule whose failure is
		 * silent — the next view written forgets it and quietly walks corpses,
		 * and the code still looks correct. Reaching for the obvious name is
		 * what makes a new pass correct.
		 *
		 * Still a LIVE view: destroying is safe now, but adding or removing an
		 * iterated component type mid-iteration is not (playbook §8.8). A loop
		 * that hands control to gameplay must snapshot ids first.
		 */
		template<typename... Components>
		auto GetLiveEntitiesWith()
		{
			return m_Registry.view<Components...>(entt::exclude<PendingDestroyComponent>);
		}

		/**
		 * As GetLiveEntitiesWith, but INCLUDING entities already marked for
		 * destruction. The deliberate exception, and anything gameplay-visible
		 * wants the other one.
		 *
		 * THIS HAS EXACTLY TWO CALLERS BY DESIGN — the reap, which exists to walk
		 * corpses, and ~Level, which marks everything including what is already
		 * marked. That is a greppable invariant, and it is written down because
		 * the seam was nearly lost the day it was built: RAD-97 migrated every
		 * pass in Level.cpp and missed LevelSerializer, which is a `friend` and
		 * had been reaching m_Registry directly. Being a friend of Level is
		 * permission to reach the registry, not a reason to bypass this. When
		 * auditing, check the friend list — not just this file.
		 */
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		/**
		 * Has this entity been marked destroyed but not yet reaped? The single
		 * predicate behind the pending half of Entity::IsValid(), which composes
		 * it with the registry's own validity — two separate questions that stay
		 * separate here (RAD-97).
		 *
		 * Sits on the hot path: IsValid() is behind every operator bool and
		 * every checked component accessor. A tag-pool lookup is a page index
		 * plus one array read, and the pool carries no value array, so this is
		 * about as cheap as asking the registry anything gets.
		 */
		bool IsPendingDestroy(entt::entity handle) const { return m_Registry.all_of<PendingDestroyComponent>(handle); }

		/**
		 * Is DestroyEntity currently on the stack for this entity? A SEPARATE
		 * question from IsPendingDestroy above, and keeping the two apart is a
		 * deliberate design decision rather than an accident (RAD-97).
		 *
		 * The tag says "dead" and is what IsValid() reads; this says "the mark
		 * is running", and exists only to stop `GetOwner().Destroy()` inside a
		 * behaviour's own OnDestroy from re-entering forever. They cannot be one
		 * flag, because the recursion guard has to be set BEFORE OnDestroy while
		 * the death marker has to be set AFTER it: a dying script that spawns an
		 * effect at its own location is the ordinary case, and it needs to still
		 * be able to read itself.
		 *
		 * Unreal splits the same pair for the same reason — bActorIsBeingDestroyed
		 * is set before Destroyed() purely to "prevent recursion"
		 * (LevelActor.cpp:908), while the flag IsValid(Object) reads is set at
		 * the very end of DestroyActor.
		 *
		 * A vector scanned linearly, not a set: this is only non-empty while a
		 * mark is executing, nesting is a call stack (so it is strictly LIFO and
		 * usually depth 0 or 1), and at that size a scan beats hashing.
		 */
		bool IsMarkInProgress(entt::entity handle) const
		{
			return std::find(m_MarksInProgress.begin(), m_MarksInProgress.end(), handle) != m_MarksInProgress.end();
		}

		/**
		 * THE REAP: frees every entity marked since the last one — behaviour
		 * instance first, then the entt row — and re-sorts once at the end.
		 * Called from exactly one place, the tail of OnFixedUpdate, which is
		 * what makes destruction a defined moment rather than whenever a caller
		 * happened to ask (playbook §1).
		 *
		 * Runs NO gameplay: OnDestroy already fired at the mark, so this only
		 * deletes. That is why a single pass suffices — nothing here can mark
		 * anything new, short of a behaviour destructor doing gameplay, and such
		 * an entity would simply be reaped on the next step, invalid throughout.
		 *
		 * Returns how many entities it freed, which ~Level loops on; the
		 * per-step caller ignores it.
		 */
		size_t ReapDestroyedEntities();

		/**
		 * Takes ownership of an already-constructed behaviour and files it: the
		 * side table, the tag, and the walk order — all three written here and
		 * nowhere else. That single-writer property is the whole of D3's
		 * no-desync argument, and it is why Entity::AddBehaviour<T> constructs
		 * the instance but does not file it: the type-specific half is templated,
		 * the bookkeeping half must not be duplicated per type.
		 *
		 * Wires the instance's owning Entity before returning, so OnCreate (which
		 * runs later, at the script pass) already has GetOwner(). Returns a
		 * NON-OWNING observer; the Level remains the sole deleter and frees only
		 * at the reap.
		 */
		EntityBehaviour* AttachBehaviour(entt::entity handle, Scope<EntityBehaviour> instance);

		/**
		 * Detach's MARK half (RAD-101 D12). Runs OnDestroy if one is owed, then
		 * flags the instance for the reap. Frees nothing and erases nothing —
		 * every sibling is still alive when OnDestroy runs, exactly as at the
		 * entity mark, and the vector is not compacted because a walk may be
		 * holding indices into it.
		 *
		 * Idempotent: a second detach of the same instance is a silent no-op,
		 * matching DestroyEntity's already-condemned early-out.
		 */
		void DetachBehaviour(EntityBehaviour& instance);

		/**
		 * Has this behaviour been detached and not yet reaped? The single
		 * predicate behind the skip in every dispatch loop and in the scan below,
		 * so the rule cannot be re-implemented per caller (playbook §4).
		 *
		 * Non-templated and defined out of line ON PURPOSE, and the reason is a
		 * C++ rule worth knowing: reading m_PendingRemove inside FindBehaviour<T>
		 * would be a NON-DEPENDENT expression on Scope<EntityBehaviour>, which the
		 * compiler checks when this header is parsed rather than when the template
		 * is instantiated — and EntityBehaviour is only forward-declared here. The
		 * dynamic_cast<T*> beside it is fine precisely because it IS dependent on
		 * T. Taking the argument by reference needs no complete type; only member
		 * access does.
		 */
		bool IsBehaviourDetached(const EntityBehaviour& instance) const;

		/**
		 * THE script pass: for each entity in the walk order, OnCreate for anything
		 * not yet created, then OnUpdate — both in attach order (RAD-101 D4, D11).
		 *
		 * Every loop is bounded by a count taken before it starts, and liveness is
		 * re-checked PER BEHAVIOUR rather than per entity, because gameplay running
		 * here may attach, detach, or destroy — including the entity whose list is
		 * mid-walk. Marked entries are skipped, never erased; the reap compacts.
		 */
		void UpdateBehaviours(Timestep ts);

		/**
		 * The mark half of teardown for one entity: every attached behaviour's
		 * OnDestroy, in REVERSE insertion order, and only where one is owed
		 * (RAD-101 D6). Deletes nothing — every sibling is still fully alive while
		 * these run, which is what lets a dying behaviour legitimately look one up.
		 */
		void RunBehaviourDestroyHooks(entt::entity handle);

		/**
		 * The reap half for one entity: deletes every instance in reverse
		 * insertion order, drops the side-table entry, and removes the entity from
		 * the walk order. Runs no gameplay — OnDestroy already fired at the mark.
		 */
		void FreeBehaviours(entt::entity handle);

		/**
		 * Erases behaviours detached from entities that are still ALIVE, at the
		 * reap. Separate from FreeBehaviours because the entity survives: only the
		 * marked entries go, the surviving ones keep their relative order, and the
		 * tag comes off if the entity's last behaviour just left.
		 */
		void CompactDetachedBehaviours();

		/**
		 * The typed scan behind Entity::GetBehaviour<T> and everything built on
		 * it. Lives on Level rather than Entity for an access reason, not a
		 * stylistic one: reading m_PendingRemove requires friendship with
		 * EntityBehaviour, and Level is its only friend. Keeping the scan here
		 * also keeps one resolution helper per layer (playbook §4) instead of
		 * letting the skip rule get re-implemented per caller.
		 *
		 * Defined in EntityTemplates.h, since dynamic_cast needs the complete
		 * EntityBehaviour that this header only forward-declares.
		 */
		template<typename T>
		T* FindBehaviour(entt::entity handle) const;

		/**
		 * What a move does to the render snapshot — an enum rather than a bool
		 * because `WriteTransform(e, p, r, true, …)` says nothing at the call
		 * site (playbook §4: semantic flags are enums). It is also the ONLY
		 * axis, deliberately: "stamp the snapshot" and "push through the
		 * logging, velocity-carrying physics Teleport" both mean *this move was
		 * discontinuous*, so a second parameter for the physics half would let
		 * a caller express a state that must never exist.
		 */
		enum class SnapshotPolicy
		{
			Preserve,             // continuous — rendering interpolates toward the new pose
			StampToDestination    // discontinuous — no interpolation, so a jump does not smear
		};

		/**
		 * This layer's ONE guard (playbook §4, one resolver per layer): an
		 * invalid handle warns, naming the verb, and returns null. Everything
		 * that touches an entity's transform comes through here, so no verb
		 * carries a guard of its own and adding one cannot get it subtly wrong.
		 *
		 * Returns a pointer INTO the component pool — use it and drop it. It is
		 * invalidated by anything that emplaces into the TransformComponent
		 * pool, so never hold it across a call that can run gameplay.
		 *
		 * Pass __func__ for verb, never a literal — see the physics resolvers
		 * for the reasoning.
		 */
		TransformComponent* ResolveTransform(Entity entity, const char* verb);

		/**
		 * The ECS half every move shares: the transform write and the snapshot
		 * policy, guarded by ResolveTransform above. Returns false when the
		 * handle was invalid (and warned), so the caller skips its physics push.
		 *
		 * The physics half deliberately stays with each public verb, because
		 * that is the part that genuinely differs — Teleport needs the logging,
		 * velocity-policy push and SetTransform needs the quiet one. This
		 * helper owns what is actually common, which is also the only part that
		 * MUST live on Level: writing TransformComponent and the snapshot is
		 * registry-owner work (playbook §4, the RAD-90 split).
		 *
		 * Pass __func__ for verb, never a literal — see the physics resolvers
		 * for the full reasoning; the WARN has to name the verb the caller
		 * actually invoked, not this helper.
		 */
		bool WriteTransform(Entity entity, const glm::vec3& translation, float rotationZ, SnapshotPolicy snapshot, const char* verb);

		/**
		 * The one place a physics-only verb answers "may I touch this entity's
		 * physics?" — PhysicsBody's whole guard, so no verb carries its own
		 * (playbook §4, one resolver per layer). Returns null on an invalid
		 * handle, after a WARN naming the verb, matching DestroyEntity and
		 * Teleport; and null WITHOUT a warning on a scratch level, because
		 * having no world is a fact about the level rather than a mistake by
		 * the caller (the same contract OnFixedUpdate's null check uses).
		 *
		 * Stays private deliberately: it hands back the physics world, and the
		 * world is Level-owned state, not public API. PhysicsBody is a friend
		 * precisely so this can remain so.
		 */
		PhysicsWorld2D* ResolvePhysics(Entity entity, const char* verb);

		/**
		 * Hands the physics world's contact batch to gameplay: level-wide
		 * callbacks first (whole event), then each live side's script hook.
		 * Both participants are resolved ONCE per event and the same handles are
		 * handed to every callback — safe because a destroy only marks, so a
		 * handler killing a participant changes what those handles ANSWER rather
		 * than invalidating them (RAD-97; this used to be a re-resolution before
		 * every single call). Called from OnFixedUpdate after the move drain, so
		 * handlers read this step's transforms.
		 */
		void DispatchContactEvents();

		/** Delivers one side's collision hook, if it has a live, active script. */
		void NotifyScript(Entity entity, Entity other, ContactPhase phase);

		/** Frees a callback slot's function and retires every handle to it. */
		void ReleaseCallbackSlot(uint32_t index);

		void OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentDestroy(entt::registry& registry, entt::entity entity);

		// Keeps MetadataComponent pool order deterministic (creation order) after
		// destruction/deserialization, so iteration order is stable across runs
		void SortEntities();

	private:
		// This level's own physics world — created in the constructor for live
		// levels, null for scratch levels (initialize == false), destroyed with
		// the level AFTER the destructor body's entity loop (bodies die while
		// the world is still alive). Declared BEFORE m_Registry so the world
		// also outlives the registry during member teardown — no signal can
		// ever fire into a dead world regardless of entt's destruction
		// behavior. PhysicsWorld2D is forward-declared, so the out-of-line
		// ~Level() is required for the unique_ptr to destroy it.
		Scope<PhysicsWorld2D> m_PhysicsWorld;

		entt::registry m_Registry;

		std::string m_Name;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::unordered_map<UUID, Entity> m_EntityMap;

		// THE BEHAVIOUR SIDE TABLE (RAD-101). Every EntityBehaviour instance in
		// this level lives here rather than in a component: an instance is
		// neither trivially copyable nor serializable, so it fails the plain-data
		// rule components must satisfy (playbook §3). What remains in the ECS is
		// the empty BehaviourComponent tag, which exists for one reason only —
		// an entt view cannot consult a std::unordered_map, and OnFixedUpdate's
		// snapshot pass classifies movers by component.
		//
		// Scope, so there is exactly one owner and exactly one deleter (playbook
		// §2): this Level. Instances are freed ONLY in ReapDestroyedEntities,
		// never in DestroyEntity — which merely runs OnDestroy — and that split
		// is what makes a behaviour destroying its own entity safe.
		//
		// The vector's order IS the update order for one entity: forward for
		// OnCreate/OnUpdate, reverse for OnDestroy. A mid-walk attach may grow
		// it, which moves the Scopes but NOT the objects they point at, so an
		// EntityBehaviour* held across a gameplay callback survives the
		// reallocation. That address stability is why instances are pointed-to
		// rather than stored by value, and it is load-bearing for the walk.
		std::unordered_map<entt::entity, std::vector<Scope<EntityBehaviour>>> m_Behaviours;

		// The CROSS-ENTITY walk order: every entity holding at least one
		// behaviour, in first-attach order. Maintained — appended at an entity's
		// first attach, dropped at the reap — rather than rebuilt from a view
		// each step, which is the shape UE's tick registry already has.
		//
		// A separate list rather than iterating m_Behaviours, and that is the
		// entire reason it exists: unordered_map iteration order is unspecified
		// and reshuffles on rehash, so a walk over it could never promise the
		// defined order this card owes. Note it replaces something WEAKER, not
		// stronger — today's order is the NativeScriptComponent pool's, i.e.
		// attach order perturbed by swap-and-pop every time an entity dies,
		// because SortEntities only ever sorts the metadata pool.
		std::vector<entt::entity> m_BehaviourEntities;

		// Entities holding at least one DETACHED behaviour awaiting compaction —
		// recorded by DetachBehaviour, drained by the reap (RAD-101 D12). The same
		// record-now-act-at-a-defined-point shape as m_ReapList and for the same
		// reason (playbook §1): the walk may be holding indices into the very
		// vector compaction would shift.
		//
		// A LIST rather than a per-step scan of m_BehaviourEntities, so the cost
		// is proportional to detaches rather than to population. Duplicate entries
		// are harmless and not filtered: two detaches on one entity in one step
		// append twice, and the second compaction pass simply finds nothing left
		// to remove.
		std::vector<entt::entity> m_BehaviourCompactList;

		// Set whenever an entity's whole behaviour list leaves m_Behaviours, by
		// either route (the entity died, or its last behaviour was detached).
		// Cleared by the one walk-order sweep in the reap.
		//
		// A flag rather than sweeping unconditionally, because the sweep is O(n)
		// over behaviour-carrying entities and the overwhelmingly common step
		// changes nothing — an unconditional scan would add a per-step cost where
		// there was none, to tidy a list that did not move.
		bool m_BehaviourWalkOrderDirty = false;

		// Entities whose DestroyEntity call is currently on the stack — the
		// recursion guard described at IsMarkInProgress. Pushed before OnDestroy
		// runs and popped when the mark returns, so it is empty outside a mark
		// and never grows beyond the nesting depth of destroys-within-OnDestroy.
		std::vector<entt::entity> m_MarksInProgress;

		// The entities to free this reap, snapshotted from the PendingDestroy
		// pool before the first one is destroyed. Same two reasons as
		// m_BehaviourCompactList: a member so a steady state of dying entities
		// allocates nothing, and plain ids rather than a live view because
		// m_Registry.destroy swap-and-pops the pool being walked (playbook §8.8)
		// — the reap must obey the rule it exists to relieve everyone else of.
		std::vector<entt::entity> m_ReapList;

		// Level-wide collision callbacks. Slot storage plus a free list, the
		// same pool shape TimerManager uses: slots never shrink, so an index
		// stays meaningful forever and the generation counter retires handles
		// to a recycled slot.
		struct CollisionCallback
		{
			// Named Function, not Callback: this struct IS the callback, and a
			// CollisionCallback::Callback reads as though it held another one
			std::function<void(const CollisionEvent&)> Function;
			uint32_t Generation = 0;
			bool Active = false;
		};

		// deque, NOT vector, and this is load-bearing: registering a callback
		// from inside a collision callback is documented as legal, and a vector
		// growing past capacity would free the buffer holding the std::function
		// that is mid-call — a use-after-free on return. Deque insertion
		// invalidates iterators but never references to existing elements, and
		// indexing stays O(1).
		std::deque<CollisionCallback> m_CollisionCallbacks;
		std::vector<uint32_t> m_FreeCallbackSlots;       // released slots awaiting reuse
		std::vector<uint32_t> m_PendingCallbackReleases; // removals deferred until dispatch ends

		// True only while DispatchContactEvents is walking a batch. Two jobs:
		// it defers callback releases (a callback may be removing itself), and
		// it is the tripwire for a handler re-entering the fixed update — which
		// would Step the world again and clear the buffer being walked.
		bool m_DispatchingContacts = false;

		// True only while the behaviour pass is running. The tripwire for
		// re-entering OnFixedUpdate from gameplay: re-entry would start a second
		// walk over m_BehaviourEntities and the same behaviour lists, double-
		// dispatching OnUpdate and stepping physics twice in one step. A script
		// cannot reach OnFixedUpdate (that is what GameplayLevel is for), so this
		// guards the remaining path — the owner calling it re-entrantly.
		bool m_RunningScripts = false;

		// For Debugging Purposes
		bool m_ShowPhysicsColliders = true;

		friend class Entity;
		// Reaches ResolvePhysics (above) and nothing else. The alternative was
		// a public physics-world accessor, which would expose Level-owned
		// state to every consumer to serve one type in this same module.
		friend class PhysicsBody;
		friend class LevelSerializer;
	};

}

#include "EntityTemplates.h"
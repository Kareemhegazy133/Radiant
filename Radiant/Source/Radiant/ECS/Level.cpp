#include "Radiant/rdpch.h"
#include "Level.h"

#include "Radiant/Asset/AssetManager.h"

#include "Radiant/Renderer/Renderer2D.h"

#include "Entity.h"
#include "Components.h"
//#include "Radiant/GAS/AbilitySystemComponent.h"
#include "Radiant/Gameplay/EntityBehaviour.h"

#include "Radiant/Physics/PhysicsWorld2D.h"

namespace Radiant {

	// Debug view of a box collider, drawn from ECS data only (never from Box2D
	// state). Lives here rather than in the physics module so Physics/ carries
	// no renderer dependency. Must be called inside a Renderer2D scene.
	static void DebugDrawCollider(const TransformComponent& tc, const BoxCollider2DComponent& bc2d)
	{
		glm::vec3 translation = tc.Translation + glm::vec3(bc2d.Offset, 0.001f);
		glm::vec3 scale = tc.Scale * glm::vec3(bc2d.Size * 2.0f, 1.0f);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
			* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), scale);

		Renderer2D::DrawRect(transform, glm::vec4(1.0f, 0.5f, 0.7f, 1.0f));
	}

	Level::Level(const std::string& name, bool initialize)
		: m_Name(name)
	{
		if (!initialize) return;

		RADIANT_TRACE("Level Constructor");
		m_PhysicsWorld = CreateScope<PhysicsWorld2D>(m_Name);

		// entt construct/destroy signals
		// Components with signals registered here should be explicitly removed from the entity
		// in DestroyEntity() before the entity is itself destroyed. This ensures that the on_destroy()
		// handlers will be called before the entity is destroyed (in particular before the entity's
		// MetadataComponent and TransformComponent are destroyed)
		m_Registry.on_construct<RigidBody2DComponent>().connect<&Level::OnRigidBody2DComponentConstruct>(this);
		m_Registry.on_destroy<RigidBody2DComponent>().connect<&Level::OnRigidBody2DComponentDestroy>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Level::OnBoxCollider2DComponentConstruct>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().connect<&Level::OnBoxCollider2DComponentDestroy>(this);

		RADIANT_TRACE("Level Constructed: {0}", (void*)this);
	}

	Level::~Level()
	{
		RADIANT_TRACE("Level Destructor");

		// Mark every entity, then reap. Marking one by one rather than calling
		// m_Registry.clear() ensures component on_destroy signals fire in the
		// correct order. Bodies die here, via the signals, while m_PhysicsWorld
		// is still alive; the Scope then destroys the world itself after this
		// destructor body. Scratch levels hold a null Scope — their destruction
		// touches no other Level's physics (the RAD-27 fix).
		//
		// This loop is now SAFE BY CONSTRUCTION, where before RAD-97 it destroyed
		// and re-sorted the very MetadataComponent pool it was iterating (half of
		// RAD-81's finding). The mark touches the collider pool, the rigidbody
		// pool, the tag pool and a hash map — never the pool being walked.
		for (auto entity : GetAllEntitiesWith<MetadataComponent>())
		{
			DestroyEntity({ entity, this });
		}

		// A loop, unlike the single pass OnFixedUpdate runs: teardown is not a hot
		// path, and an OnDestroy that destroys something else must not strand an
		// entity whose own OnDestroy never runs. Normally it converges in one or
		// two passes, because marking is idempotent and the loop above already
		// marked everything.
		//
		// CAPPED anyway. The convergence argument assumes gameplay creates nothing
		// during teardown, which is an assumption about game code rather than
		// something the engine enforces — a behaviour destructor that spawns and
		// destroys would spin here forever. A cap turns an unkillable hang into a
		// diagnosable line, and anything still pending is released by the registry
		// itself when it is destroyed moments later.
		constexpr int maxReapPasses = 8;
		int passes = 0;
		while (ReapDestroyedEntities() > 0)
		{
			if (++passes >= maxReapPasses)
			{
				RADIANT_WARN("Level: teardown reap did not converge after {0} passes - something is creating entities during ~Level", maxReapPasses);
				break;
			}
		}

		RADIANT_TRACE("Level Destructed: {0}", (void*)this);
	}

	Entity Level::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Level::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };

		entity.AddComponent<TransformComponent>();
		auto& metadata = entity.AddComponent<MetadataComponent>();
		metadata.ID = uuid;
		metadata.Tag = name.empty() ? "Entity" : name;

		// A colliding UUID (corrupt/hand-edited level data) silently orphans the
		// previous entity's map entry — say so instead of swallowing it
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			RADIANT_WARN("Level: duplicate entity UUID {} on create - previous map entry replaced", uuid);

		m_EntityMap[uuid] = entity;

		return entity;
	}

	// THE MARK (RAD-97). Nothing here frees anything: the entt row and the
	// behaviour instance both die at the reap, which is what makes this callable
	// from a collision handler, from a script's OnUpdate, or by an entity on
	// itself, with no "last statement" discipline anywhere.
	void Level::DestroyEntity(Entity entity)
	{
		const entt::entity handle = entity.m_EntityHandle;

		// The old IsValid() body, spelled out, because IsValid() now folds in the
		// pending check and the two cases want opposite answers below: a handle
		// naming no row is a caller mistake, an already-condemned one is not.
		if (handle == entt::null || !entity.m_Level || !m_Registry.valid(handle))
		{
			RADIANT_WARN("Level: DestroyEntity called with an invalid entity handle");
			return;
		}

		// Already condemned. SILENT, deliberately: the caller is asking for a
		// state that already holds, and two systems independently concluding the
		// same enemy is dead in one step is ordinary gameplay, not a bug — a
		// warning here would fire during correct behaviour, which is how a log
		// gets tuned out. UE takes the identical early-out, and says so: "If
		// already on list to be deleted, pretend the call was successful"
		// (LevelActor.cpp:856).
		if (IsPendingDestroy(handle))
			return;

		// Same answer, different question — see IsMarkInProgress in Level.h for
		// why these are two states and not one. This is the arm that catches
		// GetOwner().Destroy() called from inside this entity's own OnDestroy.
		if (IsMarkInProgress(handle))
			return;

		{
			// Scoped like DispatchScope and ScriptPassScope below, and for the
			// same reason: OnDestroy is gameplay, and gameplay that throws must
			// not leave this entity permanently unmarkable.
			struct MarkScope
			{
				Level& Owner;

				MarkScope(Level& owner, entt::entity handle) : Owner(owner) { Owner.m_MarksInProgress.push_back(handle); }
				~MarkScope() { Owner.m_MarksInProgress.pop_back(); }
			} markScope(*this, handle);

			// Every attached behaviour's OnDestroy, LIFO (RAD-101 D6). Nothing is
			// deleted here: that delete was the entire "last statement" wart and it
			// lives at the reap now. What a behaviour sees is otherwise exactly what
			// it saw before — its entity valid, its components readable, its body
			// still alive, and every SIBLING still alive — which is what keeps
			// "spawn an effect where I died" working, and what lets a dying
			// behaviour legitimately read one of its siblings.
			//
			// Inside the MarkScope, so GetOwner().Destroy() from any of these bodies
			// still hits the recursion guard. With N behaviours that guard covers
			// the whole list at once rather than one instance.
			RunBehaviourDestroyHooks(handle);
		}

		// OnDestroy is gameplay, so it may have spawned (reallocating the pool
		// `nsc` pointed into) or destroyed other entities. Nothing below reuses
		// nsc, and `handle` itself stays meaningful because no row is freed until
		// the reap.

		const UUID id = entity.GetUUID();

		// Remove components for which there exist on_destroy handlers. This
		// ensures that if the handlers rely on other entity components (in
		// particular the MetadataComponent and the TransformComponent), they can
		// still access them — which is also why this runs BEFORE the tag: the
		// handlers reach the entity through checked accessors, and the tag is
		// what would make those assert.
		// Collider before rigidbody: destroying a body destroys its shapes inside
		// Box2D, so body-first would leave the collider handler a stale ticket.
		entity.RemoveComponentIfExists<BoxCollider2DComponent>();
		entity.RemoveComponentIfExists<RigidBody2DComponent>();

		// The level's directory entry goes now, so GetEntityByUUID and the UUID
		// overload of this function stop finding it without either needing a
		// pending check of its own. UE strikes its actor-list entry inside
		// DestroyActor for the same reason (RemoveActor, LevelActor.cpp:1033).
		m_EntityMap.erase(id);

		// LAST. From here the entity is dead to everything: IsValid() is false,
		// every gameplay view excludes it, every lookup misses it. Everything
		// above needed it alive, and the recursion guard is what covered that gap.
		m_Registry.emplace<PendingDestroyComponent>(handle);

		// No SortEntities here: it moves to the reap, where it runs once per step
		// instead of once per destroyed entity. Its comparator also resolves
		// metadata IDs through m_EntityMap, which no longer holds this one.
	}

	// THE REAP (RAD-97): the one defined point at which entity storage is freed.
	// Called from the tail of OnFixedUpdate — see Level.h's lifecycle contract
	// for why end-of-step rather than end-of-frame.
	size_t Level::ReapDestroyedEntities()
	{
		RADIANT_PROFILE_FUNCTION();

		// FIRST, and unconditionally, because it must not sit below the
		// nothing-died early-out: detaching a behaviour from an entity that goes on
		// living is the common case for RemoveBehaviour, and on such a step the
		// reap list is empty. Putting this after the early return would leave every
		// detached instance alive forever — a leak that only shows up as a slowly
		// growing walk.
		//
		// Entities that died this step are handled by FreeBehaviours below instead;
		// compacting one here first is merely redundant, never wrong.
		CompactDetachedBehaviours();

		// Snapshot to plain ids before destroying any of them: m_Registry.destroy
		// swap-and-pops the very pool this list came from, so the reap has to obey
		// the rule it exists to relieve every other caller of (playbook §8.8).
		// GetAllEntitiesWith, not GetLiveEntitiesWith — this is one of the two
		// places that deliberately walks corpses.
		m_ReapList.clear();
		for (entt::entity handle : GetAllEntitiesWith<PendingDestroyComponent>())
			m_ReapList.push_back(handle);

		// NOTE the early `return 0` that used to sit here is gone, and deliberately:
		// a detach on a step where nothing DIED still has a walk order to reconcile
		// below, and returning early skipped it — leaving the detached entity in the
		// walk forever. The two costs the early-out protected are now guarded
		// individually (the reap loop by m_ReapList, SortEntities by `reaped`), so
		// the quiet step is still as cheap as it was.

		// Counted rather than taken from m_ReapList.size(), and that difference is
		// load-bearing: ~Level loops `while (ReapDestroyedEntities() > 0)`, so an
		// entity that the guard below SKIPS would keep its tag, be re-listed next
		// pass, and spin forever. Reporting work actually done makes the loop
		// terminate on its own terms instead of on an invariant argument.
		size_t reaped = 0;

		for (entt::entity handle : m_ReapList)
		{
			// Only the reap frees rows, and it cannot run re-entrantly, so a
			// missing row means something freed an entity behind the registry's
			// back — a programmer error, not content
			RADIANT_ASSERT(m_Registry.valid(handle), "Reap: entity already freed outside DestroyEntity");
			if (!m_Registry.valid(handle))
				continue;

			// The deferred half of the mark, and the whole fix for the "last
			// statement" wart: by the time this runs, every method that was
			// executing when Destroy() was called has returned. Uses m_Registry
			// directly — these entities are, by construction, invalid, and
			// Entity's accessors assert on that.
			//
			// One call per kind of runtime side state, and there is exactly one kind
			// today. This is where the bespoke NativeScriptComponent delete used to
			// sit, and its comment promised this replacement to RAD-30/RAD-101; the
			// loop over registered tables it also anticipated is still not needed,
			// because one table is not three. It becomes a loop at the rule of
			// three, not in advance of it.
			FreeBehaviours(handle);

			m_Registry.destroy(handle);
			++reaped;
		}

		m_ReapList.clear();

		// THE WALK ORDER, RECONCILED IN ONE PASS — and the "once per reap rather
		// than once per destroyed entity" rule below is exactly why it lives here.
		// An erase inside FreeBehaviours would be an O(n) scan-and-shift per dead
		// entity, so fifty projectiles dying in one step would be fifty of them:
		// the same O(M·N) shape SortEntities was hoisted out of that loop to
		// escape, rebuilt in a different container.
		//
		// It is also one fewer thing to keep in sync. The order is DERIVED from the
		// table rather than maintained beside it, so an entity is in the walk
		// exactly while it has behaviours — which covers both ways it can leave
		// (destroyed, or its last behaviour detached) without either path having to
		// know this list exists. erase_if preserves the relative order of the
		// survivors, which is the whole reason the container is a vector and not a
		// set.
		if (m_BehaviourWalkOrderDirty)
		{
			std::erase_if(m_BehaviourEntities, [this](entt::entity handle)
				{
					return m_Behaviours.find(handle) == m_Behaviours.end();
				});
			m_BehaviourWalkOrderDirty = false;
		}

		// ONCE per reap rather than once per destroyed entity, which is the
		// single largest cost RAD-97 removed: fifty projectiles dying in one
		// step was fifty O(n log n) sorts. Safe here and nowhere else in the
		// window, because the comparator resolves every metadata ID through
		// m_EntityMap and asserts on a miss — by this line no pending entity
		// remains to be missing.
		//
		// Guarded on `reaped` now that the nothing-died early-out above is gone: a
		// step that only detached a behaviour has not moved a single entity id, so
		// sorting would be pure cost. This keeps the quiet step exactly as cheap as
		// the early return made it.
		if (reaped > 0)
			SortEntities();

		return reaped;
	}

	EntityBehaviour* Level::AttachBehaviour(entt::entity handle, Scope<EntityBehaviour> instance)
	{
		RADIANT_ASSERT(instance, "Level::AttachBehaviour: null instance");
		if (!instance)
			return nullptr;

		// Wired BEFORE the instance is reachable by anything, so OnCreate — which
		// runs later, at the script pass — already has a usable GetOwner()
		instance->m_Entity = Entity{ handle, this };

		std::vector<Scope<EntityBehaviour>>& list = m_Behaviours[handle];

		// The entity joins the walk order on its FIRST behaviour only. Testing the
		// list rather than the tag keeps the table authoritative and the tag
		// derived, which is the direction that cannot desync.
		if (list.empty())
		{
			m_BehaviourEntities.push_back(handle);

			// Through m_Registry, not through Entity: BehaviourComponent is
			// IsEngineComponent, and RAD-97's rule is that an engine path which
			// legitimately writes a gated component uses the registry rather than
			// widening the gate. emplace_or_replace rather than emplace so a
			// stale tag (which would mean the invariant was already broken
			// elsewhere) cannot turn this into an entt assert.
			m_Registry.emplace_or_replace<BehaviourComponent>(handle);
		}

		list.push_back(std::move(instance));
		return list.back().get();
	}

	bool Level::IsBehaviourDetached(const EntityBehaviour& instance) const
	{
		return instance.m_PendingRemove;
	}

	void Level::DetachBehaviour(EntityBehaviour& instance)
	{
		// Already marked. Silent, for DestroyEntity's reason: the caller is asking
		// for a state that already holds, and a warning here would fire during
		// correct gameplay.
		if (instance.m_PendingRemove)
			return;

		// Only if one is owed. A behaviour attached and detached before the walk
		// ever reached it never ran OnCreate, and must not receive an OnDestroy
		// for it — the pairing UE enforces with HasBeenInitialized() before
		// calling UninitializeComponent (Actor.cpp:6350-6362).
		if (instance.m_HasCreated)
		{
			instance.OnDestroy();
			instance.m_HasCreated = false;
		}

		// LAST, and after OnDestroy rather than before: OnDestroy is gameplay and
		// may legitimately look itself up (GetOwner().GetBehaviour<T>()), which the
		// scan skips once this is set.
		instance.m_PendingRemove = true;

		// Recorded for the reap to erase. Not erased here for the same reason
		// DestroyEntity does not free a row here: a walk may be holding an index
		// into this very vector, and compaction shifts everything after the hole.
		m_BehaviourCompactList.push_back(instance.m_Entity.m_EntityHandle);
	}

	void Level::UpdateBehaviours(Timestep ts)
	{
		// Captured BEFORE the walk, never re-read as `i < size()`. Gameplay may
		// attach to a brand-new entity and append here, and such a behaviour must
		// wait for the next step — RAD-95's existing spawn contract, which
		// SpawnProbe already demonstrates for entities. Under the old per-step
		// snapshot that was automatic; over a maintained list it is a choice, and
		// the accidental spelling changes a documented contract in silence.
		//
		// Indexing rather than iterating, because push_back may reallocate. For the
		// same reason nothing below caches a reference into m_BehaviourEntities.
		const size_t entityCount = m_BehaviourEntities.size();

		for (size_t ei = 0; ei < entityCount; ++ei)
		{
			const entt::entity handle = m_BehaviourEntities[ei];

			// A live check, not bare registry validity: the row survives until the
			// reap, so an entity destroyed by an earlier behaviour in this same
			// pass is still present and must still be skipped (playbook §8.8)
			if (!m_Registry.valid(handle) || IsPendingDestroy(handle))
				continue;

			auto it = m_Behaviours.find(handle);
			if (it == m_Behaviours.end())
				continue;

			// A REFERENCE into the map, held across gameplay deliberately: an
			// unordered_map keeps references to its mapped values valid through a
			// rehash (only iterators die), so attaching to some OTHER entity cannot
			// invalidate this. Attaching to THIS one may reallocate the vector's
			// buffer, which is why each access below re-reads list[i] rather than
			// caching a pointer into it.
			std::vector<Scope<EntityBehaviour>>& list = it->second;
			const size_t count = list.size();

			// OnCreate first, for the whole entity, before any of its OnUpdates —
			// the within-entity guarantee composition actually needs, since this is
			// where siblings can see each other. Deliberately NOT a level-wide
			// guarantee: entity A's OnCreate still follows entity B's OnUpdate when
			// B sorts earlier, exactly as today (RAD-101 D11).
			for (size_t i = 0; i < count; ++i)
			{
				EntityBehaviour* behaviour = list[i].get();
				if (behaviour->m_PendingRemove || behaviour->m_HasCreated)
					continue;

				behaviour->OnCreate();
				behaviour->m_HasCreated = true;

				// OnCreate is gameplay and may have destroyed this very entity.
				// Re-checked per behaviour rather than once per entity, because a
				// sibling condemning the entity must stop the rest of ITS list too.
				if (!m_Registry.valid(handle) || IsPendingDestroy(handle))
					break;
			}

			if (!m_Registry.valid(handle) || IsPendingDestroy(handle))
				continue;

			// The same gate the legacy pass uses: an inactive entity's behaviours
			// stop updating while its body keeps simulating
			if (!m_Registry.get<MetadataComponent>(handle).IsActive)
				continue;

			for (size_t i = 0; i < count; ++i)
			{
				EntityBehaviour* behaviour = list[i].get();

				// Skipped, never erased: compaction is the reap's job, and an erase
				// here would shift every index after it out from under this loop
				if (behaviour->m_PendingRemove)
					continue;

				// A behaviour attached during THIS pass has not had OnCreate yet —
				// it waits for the next step, so it must not receive OnUpdate first
				if (!behaviour->m_HasCreated)
					continue;

				behaviour->OnUpdate(ts);

				// A sibling may have destroyed the entity, or detached itself. The
				// entity check stops the list; the per-behaviour m_PendingRemove
				// check above handles the detach case on the next iteration.
				if (!m_Registry.valid(handle) || IsPendingDestroy(handle))
					break;
			}
		}
	}

	void Level::RunBehaviourDestroyHooks(entt::entity handle)
	{
		auto it = m_Behaviours.find(handle);
		if (it == m_Behaviours.end())
			return;

		std::vector<Scope<EntityBehaviour>>& list = it->second;

		// REVERSE insertion order (RAD-101 D6): a behaviour attached later may have
		// found an earlier one in its OnCreate, so the dependent dies before its
		// dependency — the discipline every reader already has from stack
		// unwinding. We are stricter than UE here, cheaply: AActor::
		// UninitializeComponents walks a TSet and so cannot define an order at all
		// (Actor.cpp:6350-6362); our list is ordered anyway.
		//
		// Indexed downward rather than with reverse iterators because OnDestroy is
		// gameplay and may attach (appending, which can reallocate). Anything it
		// appends is NOT visited here: it never ran OnCreate, so it is owed no
		// OnDestroy, and the reap frees it regardless.
		for (size_t i = list.size(); i-- > 0; )
		{
			EntityBehaviour* behaviour = list[i].get();

			// Owed only if OnCreate actually ran. This is the pairing UE enforces
			// with HasBeenInitialized() before calling UninitializeComponent, and
			// it is what makes attach-then-destroy-before-the-first-step run
			// neither hook rather than the wrong one.
			if (!behaviour->m_HasCreated || behaviour->m_PendingRemove)
				continue;

			behaviour->OnDestroy();
			behaviour->m_HasCreated = false;
		}
	}

	void Level::FreeBehaviours(entt::entity handle)
	{
		auto it = m_Behaviours.find(handle);
		if (it == m_Behaviours.end())
			return;

		// Reverse insertion order here too, so destructors mirror the OnDestroy
		// order the mark already used. pop_back rather than clear() so the sequence
		// is explicit rather than an implementation detail of vector.
		std::vector<Scope<EntityBehaviour>>& list = it->second;
		while (!list.empty())
			list.pop_back();

		// BY KEY, not by the iterator above, and the difference is not stylistic:
		// pop_back ran behaviour DESTRUCTORS, and a destructor that attaches
		// anything rehashes this map and invalidates `it`. Erasing an invalidated
		// iterator is undefined behaviour, where one extra hash costs nothing.
		// ~Level already contemplates exactly this actor ("something is creating
		// entities during ~Level").
		m_Behaviours.erase(handle);

		// The walk order is deliberately NOT edited here, only flagged — see the
		// single sweep at the tail of ReapDestroyedEntities for why.
		m_BehaviourWalkOrderDirty = true;
	}

	void Level::CompactDetachedBehaviours()
	{
		if (m_BehaviourCompactList.empty())
			return;

		for (entt::entity handle : m_BehaviourCompactList)
		{
			auto it = m_Behaviours.find(handle);
			if (it == m_Behaviours.end())
				continue;   // the whole entity died this step; FreeBehaviours already took it

			std::vector<Scope<EntityBehaviour>>& list = it->second;

			// erase-remove, which PRESERVES the relative order of the survivors —
			// the same requirement the walk order has, one level down
			std::erase_if(list, [this](const Scope<EntityBehaviour>& held)
				{
					return held && IsBehaviourDetached(*held);
				});

			// The entity loses its tag together with its last behaviour. Missing
			// this is how the tag outlives the table: an entity the snapshot pass
			// still treats as a mover with nothing behind it (RAD-101 D3, D9 — the
			// invariant the trait protects).
			if (list.empty())
			{
				// By key, not by `it`: erase_if above ran behaviour destructors, and
				// one that attaches would rehash this map. Same reasoning as
				// FreeBehaviours.
				m_Behaviours.erase(handle);
				m_BehaviourWalkOrderDirty = true;

				// Through m_Registry because the tag is IsEngineComponent, per
				// RAD-97's rule that an engine path uses the registry rather than
				// widening the gate
				if (m_Registry.valid(handle))
					m_Registry.remove<BehaviourComponent>(handle);
			}
		}

		m_BehaviourCompactList.clear();
	}

	void Level::DestroyEntity(UUID entityID)
	{
		auto it = m_EntityMap.find(entityID);
		if (it == m_EntityMap.end())
			return;

		DestroyEntity(it->second);
	}

	TransformComponent* Level::ResolveTransform(Entity entity, const char* verb)
	{
		// Stale/null handles reach here from gameplay — a content-level
		// mistake (same contract as DestroyEntity), not grounds for UB. Living
		// here rather than in each verb is what keeps the guard from drifting
		// as verbs are added (playbook §4, one resolver per layer).
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: {0} called with an invalid entity handle", verb);
			return nullptr;
		}

		return &entity.GetComponent<TransformComponent>();
	}

	bool Level::WriteTransform(Entity entity, const glm::vec3& translation, float rotationZ, SnapshotPolicy snapshot, const char* verb)
	{
		TransformComponent* resolved = ResolveTransform(entity, verb);
		if (!resolved)
			return false;

		TransformComponent& transform = *resolved;
		transform.Translation = translation;
		transform.Rotation.z = rotationZ;

		// THE WHOLE REASON THIS HELPER TAKES A POLICY. OnRender draws
		// lerp(snapshot, current, alpha), so the snapshot decides whether the
		// move is interpolated across the rendered frames between two
		// simulation steps:
		//
		//   StampToDestination — snapshot == current, so the lerp is a no-op
		//     and the entity simply appears at the destination. Correct for a
		//     jump, which must not smear across a frame.
		//   Preserve — the snapshot still holds where the entity was at the
		//     start of this step, so rendering blends toward the new pose.
		//     Correct for continuous movement; stamping here instead would
		//     pin the entity to fixed-step granularity and it would visibly
		//     judder at any display rate above the simulation rate.
		//
		// A caller that wants Preserve on an entity with no snapshot gets no
		// interpolation, because the snapshot pass only tracks entities it
		// classifies as movers (see OnFixedUpdate) — stamping one here to
		// compensate would fight that pass, which strips snapshots from
		// non-movers every step. RAD-30's explicit mover marker is the fix.
		if (snapshot == SnapshotPolicy::StampToDestination)
			m_Registry.emplace_or_replace<TransformSnapshotComponent>(entity.m_EntityHandle, transform.Translation, transform.Rotation);

		return true;
	}

	void Level::Teleport(Entity entity, const glm::vec3& translation, float rotationZ, TeleportType teleportType)
	{
		if (!WriteTransform(entity, translation, rotationZ, SnapshotPolicy::StampToDestination, __func__))
			return;

		// The one legitimate ECS→Box2D transform push for a JUMP (RAD-28) — it
		// logs and carries the velocity policy, which is what separates it from
		// SetTransform's quiet push below. A body-less entity has already been
		// fully moved by WriteTransform; teleportType is simply meaningless
		// without a velocity to act on.
		if (m_PhysicsWorld && entity.HasComponent<RigidBody2DComponent>())
			m_PhysicsWorld->Teleport(entity, { translation.x, translation.y }, rotationZ, teleportType);
	}

	void Level::Teleport(Entity entity, const glm::vec3& translation, TeleportType teleportType)
	{
		// Rotation is read by value, so the primary overload's own resolve
		// cannot invalidate it
		if (const TransformComponent* transform = ResolveTransform(entity, __func__))
			Teleport(entity, translation, transform->Rotation.z, teleportType);
	}

	void Level::SetTransform(Entity entity, const glm::vec3& translation, float rotationZ)
	{
		if (!WriteTransform(entity, translation, rotationZ, SnapshotPolicy::Preserve, __func__))
			return;

		// The quiet counterpart to Teleport's push: same placement, no velocity
		// policy and no TRACE, because gameplay may call this every fixed step
		// (playbook §4). Without it, this verb would be the RAD-28 trap it
		// exists to close — a transform write that a body-backed entity
		// silently discards on the next readback.
		if (m_PhysicsWorld && entity.HasComponent<RigidBody2DComponent>())
			m_PhysicsWorld->SetTransform(entity, { translation.x, translation.y }, rotationZ);
	}

	void Level::SetLocation(Entity entity, const glm::vec3& translation)
	{
		// Rotation is read by value, so the resolve inside SetTransform cannot
		// invalidate it
		if (const TransformComponent* transform = ResolveTransform(entity, __func__))
			SetTransform(entity, translation, transform->Rotation.z);
	}

	void Level::SetRotation(Entity entity, float rotationZ)
	{
		const TransformComponent* transform = ResolveTransform(entity, __func__);
		if (!transform)
			return;

		// COPIED, not passed by reference into the component it is about to
		// overwrite. SetTransform's write would otherwise self-assign through
		// an alias, and its physics push would then read x/y through a pointer
		// into a pool a preceding call may have touched. Both are benign today
		// — glm::vec3 is trivially copyable, and the snapshot stamp emplaces
		// into a DIFFERENT pool — but the benignness depends on facts about
		// other functions, which is not a property worth relying on for twelve
		// bytes.
		const glm::vec3 translation = transform->Translation;
		SetTransform(entity, translation, rotationZ);
	}

	void Level::RefreshCollider(Entity entity)
	{
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: RefreshCollider called with an invalid entity handle");
			return;
		}

		// Calling this on an entity with no collider is a programmer error —
		// the call site believes it configured a collider it never added
		auto* bc2d = entity.TryGetComponent<BoxCollider2DComponent>();
		RADIANT_ASSERT(bc2d, "RefreshCollider: entity has no BoxCollider2DComponent");
		if (!bc2d)
			return;

		if (m_PhysicsWorld)
			m_PhysicsWorld->UpdateBoxShape(entity, *bc2d);
	}

	PhysicsWorld2D* Level::ResolvePhysics(Entity entity, const char* verb)
	{
		// Stale/null handles reach here from gameplay holding a PhysicsBody a
		// step too long — the same contract DestroyEntity and Teleport carry
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: {0} called with an invalid entity handle", verb);
			return nullptr;
		}

		// Silent: a scratch level (initialize == false) legitimately has no
		// world, and saying so on every verb would be noise about a level, not
		// a diagnosis of a bug
		return m_PhysicsWorld.get();
	}

	Level::CollisionCallbackHandle Level::AddCollisionCallback(std::function<void(const CollisionEvent&)> callback)
	{
		// An empty callable would throw std::bad_function_call on the first
		// collision, far from the wiring mistake that caused it
		RADIANT_ASSERT(callback, "AddCollisionCallback called with an empty callable");
		if (!callback)
			return {};

		uint32_t index;
		if (!m_FreeCallbackSlots.empty())
		{
			index = m_FreeCallbackSlots.back();
			m_FreeCallbackSlots.pop_back();
		}
		else
		{
			index = static_cast<uint32_t>(m_CollisionCallbacks.size());
			m_CollisionCallbacks.emplace_back();
		}

		CollisionCallback& slot = m_CollisionCallbacks[index];
		slot.Function = std::move(callback);
		slot.Active = true;

		// The slot's CURRENT generation — bumped on release, which is what
		// makes every earlier handle to this slot stale
		return { index, slot.Generation };
	}

	void Level::RemoveCollisionCallback(CollisionCallbackHandle& handle)
	{
		// Out-of-range covers the default-constructed InvalidIndex too
		if (handle.Index >= m_CollisionCallbacks.size())
			return;

		CollisionCallback& slot = m_CollisionCallbacks[handle.Index];

		// Already removed, or a handle from an earlier occupant of this slot:
		// the "remove if still registered" case, not an error
		if (!slot.Active || slot.Generation != handle.Generation)
		{
			handle = {};
			return;
		}

		// Stops delivery immediately either way; only the FREEING is delicate
		slot.Active = false;

		if (m_DispatchingContacts)
		{
			// This may be the callback currently executing (one removing
			// itself). Freeing it here would destroy the running lambda's
			// captures underneath it — defer to the end of the batch.
			m_PendingCallbackReleases.push_back(handle.Index);
		}
		else
		{
			ReleaseCallbackSlot(handle.Index);
		}

		handle = {};
	}

	void Level::ReleaseCallbackSlot(uint32_t index)
	{
		CollisionCallback& slot = m_CollisionCallbacks[index];
		// Frees the captures now rather than at Level teardown — they may hold
		// Refs or own resources
		slot.Function = nullptr;
		// Every handle naming this slot is now stale
		++slot.Generation;
		m_FreeCallbackSlots.push_back(index);
	}

	void Level::NotifyScript(Entity entity, Entity other, ContactPhase phase)
	{
		// Same gate as OnUpdate: an inactive entity's body keeps colliding, its
		// script just stops hearing about it. Checked before either channel so the
		// two cannot disagree about it.
		if (!entity.GetComponent<MetadataComponent>().IsActive)
			return;

		// THE SIDE TABLE'S FAN-OUT (RAD-101 D10). Contacts are the third dispatch
		// site, and the one the plan originally missed: reaching a single instance
		// here would have left a three-behaviour entity with exactly one behaviour
		// hearing about collisions, silently and with no diagnostic.
		//
		// The loop is not merely "walk the list". RAD-29 already established that
		// gameplay runs here with the power to destroy (playbook §4: re-resolve
		// each side immediately before its callback, because an earlier callback in
		// the same batch may have destroyed it). This is the per-behaviour form of
		// that same rule — one hazard, one spelling, three sites.
		if (auto it = m_Behaviours.find(entity.m_EntityHandle); it != m_Behaviours.end())
		{
			std::vector<Scope<EntityBehaviour>>& list = it->second;
			const size_t count = list.size();

			for (size_t i = 0; i < count; ++i)
			{
				EntityBehaviour* behaviour = list[i].get();

				// Detached, or attached this step and not yet created: a collision
				// must never arrive before the behaviour's own OnCreate
				if (behaviour->m_PendingRemove || !behaviour->m_HasCreated)
					continue;

				if (phase == ContactPhase::Begin)
					behaviour->OnCollisionBegin(other);
				else
					behaviour->OnCollisionEnd(other);

				// An earlier handler in THIS list may have destroyed the entity the
				// rest of it belongs to
				if (!entity.IsValid())
					return;
			}
		}

		// A handler is allowed to destroy this very entity, and that is simply safe:
		// a destroy MARKS, and only the reap deletes the instance, so the object
		// whose method is running outlives the call (RAD-97). The local this used to
		// need — read the instance before calling, never touch it after — is gone
		// with the hazard it dodged.
	}

	void Level::DispatchContactEvents()
	{
		RADIANT_PROFILE_FUNCTION();

		// Scoped rather than a plain assignment pair: a gameplay callback that
		// throws would otherwise leave the flag set forever, wedging every
		// later fixed update on the re-entrancy assert and stranding the
		// deferred releases. Unwinding must restore the flag and still free
		// the slots that asked to be freed.
		struct DispatchScope
		{
			Level& Owner;

			explicit DispatchScope(Level& owner) : Owner(owner) { Owner.m_DispatchingContacts = true; }

			~DispatchScope()
			{
				Owner.m_DispatchingContacts = false;

				// Removals that arrived during the batch: safe now that no
				// callback is on the stack
				for (uint32_t index : Owner.m_PendingCallbackReleases)
					Owner.ReleaseCallbackSlot(index);
				Owner.m_PendingCallbackReleases.clear();
			}
		} dispatchScope(*this);

		for (const ContactEvent& contact : m_PhysicsWorld->GetContactEvents())
		{
			// ONE resolution per event, hoisted out of the callback loop below —
			// and this hoist is the change RAD-97 buys here. Before deferral, a
			// callback that destroyed a participant FREED its row, so any handle
			// resolved earlier became a dangling ticket and every callback had to
			// re-resolve both sides from their UUIDs; with 5 callbacks and 12
			// events that was 120 map lookups a step spent defending against a
			// free that had already happened.
			//
			// Now a destroy only MARKS. The Entity values below are value handles
			// that re-ask at every use rather than caching an answer, so a
			// callback destroying `A` does not invalidate the handle the next
			// callback holds — it changes what that handle ANSWERS. Resolve once,
			// hand the same event to everyone, and let each `if (collision.A)`
			// tell the truth at the moment it is asked.
			//
			// An unresolvable side arrives as UUID(0), which is never in the map,
			// so it needs no special case — and neither does an entity marked
			// earlier in this batch, since the mark erases its map entry.
			CollisionEvent collision{ GetEntityByUUID(contact.EntityA), GetEntityByUUID(contact.EntityB), contact.Phase };

			if (!collision.A && !collision.B)
				continue; // both gone — nobody left to tell

			// Level-wide callbacks first: they see the collision before
			// per-entity gameplay starts changing the world. (UE dispatches its
			// world-level handler ahead of per-actor notifies for the same
			// reason — PhysScene_Chaos.cpp:1154.)
			//
			// Indexed with the count captured now, so a callback registered by
			// another callback joins the NEXT batch rather than this one —
			// EventQueue's rule. The container is a deque precisely so that
			// such a registration cannot invalidate the callback executing
			// below (see m_CollisionCallbacks' declaration).
			const size_t callbackCount = m_CollisionCallbacks.size();
			for (size_t i = 0; i < callbackCount; ++i)
			{
				// Checked per iteration, not cached: an earlier callback may
				// have removed this one
				if (!m_CollisionCallbacks[i].Active)
					continue;

				// An earlier callback destroyed both participants, so there is
				// nobody left to tell. Re-checked per iteration because it can
				// become true mid-loop — but note this is a re-CHECK of the same
				// two handles, not a re-RESOLUTION of them: two tag lookups
				// rather than two hash lookups, and no chance of the answer and
				// the handle disagreeing.
				if (!collision.A && !collision.B)
					break;

				m_CollisionCallbacks[i].Function(collision);
			}

			// Then each side's script hook. The same handles, asked again: A's
			// handler may have destroyed B, and `collision.B` reports that
			// without anything being re-resolved.
			if (collision.A)
				NotifyScript(collision.A, collision.B, contact.Phase);

			if (collision.B)
				NotifyScript(collision.B, collision.A, contact.Phase);
		}

		// Flag clearing and the deferred-release flush happen in ~DispatchScope
	}

	void Level::OnFixedUpdate(Timestep ts)
	{
		// Re-entering the fixed update from a collision callback would Step the
		// world again, clearing the very event buffer DispatchContactEvents is
		// walking
		RADIANT_ASSERT(!m_DispatchingContacts, "Level::OnFixedUpdate re-entered from a collision callback");

		// Re-entering from the script pass is a different failure: it would
		// start a second behaviour walk while the outer one holds
		// iterators into it (RAD-95). Separate assert from the one above so the
		// message names which of the two paths did it.
		RADIANT_ASSERT(!m_RunningScripts, "Level::OnFixedUpdate re-entered from a script");

		// Snapshot movable entities BEFORE anything moves: rendering interpolates
		// between this (where the entity was) and the post-step transform (where
		// it is). "Movable" today = has physics, a camera, or a script — iterated
		// as three mover views (small sets) rather than an all-entity scan; an
		// explicit marker replaces this heuristic when other movers appear
		// (revisited with RAD-30; see plan §9).
		{
			auto snapshot = [this](entt::entity entityHandle)
			{
				const auto& transform = m_Registry.get<TransformComponent>(entityHandle);
				m_Registry.emplace_or_replace<TransformSnapshotComponent>(entityHandle, transform.Translation, transform.Rotation);
			};
			// Entities matching several mover views are written twice — idempotent
			for (auto entityHandle : GetLiveEntitiesWith<RigidBody2DComponent, TransformComponent>())
				snapshot(entityHandle);
			for (auto entityHandle : GetLiveEntitiesWith<CameraComponent, TransformComponent>())
				snapshot(entityHandle);
			// The tag's one and only customer, and the reason D3 kept a component at
			// all: a view cannot consult the side table, so "has a behaviour" has to
			// be askable as a component.
			for (auto entityHandle : GetLiveEntitiesWith<BehaviourComponent, TransformComponent>())
				snapshot(entityHandle);

			// An entity that STOPPED being movable must lose its snapshot, or
			// OnRender lerps it against that stale pose forever. Collect-then-
			// remove: removing the iterated component mid-iteration is against
			// the rules (Level.h); the vector only allocates on the rare frame
			// a mover component was actually removed.
			std::vector<entt::entity> staleSnapshots;
			for (auto entityHandle : GetLiveEntitiesWith<TransformSnapshotComponent>())
			{
				// Must list every mover category above, or an entity that is still
				// a mover loses its snapshot and stops interpolating. Adding a
				// category to the snapshot loop without adding it here is the
				// silent failure this pairing exists to prevent.
				if (!m_Registry.any_of<RigidBody2DComponent, CameraComponent, BehaviourComponent>(entityHandle))
					staleSnapshots.push_back(entityHandle);
			}
			for (auto entityHandle : staleSnapshots)
				m_Registry.remove<TransformSnapshotComponent>(entityHandle);
		}

		// Gameplay can reach CreateEntity/DestroyEntity/AddBehaviour from inside
		// the pass below (RAD-95), so nothing there may ride a live entt view or
		// an unbounded loop — UpdateBehaviours bounds every loop by a count taken
		// before it starts and re-checks liveness per behaviour (playbook §8.8).
		//
		// Scoped like DispatchScope above, and for the same reason: a gameplay
		// callback that throws must not leave the flag set and wedge every later
		// fixed update.
		struct ScriptPassScope
		{
			Level& Owner;
			explicit ScriptPassScope(Level& owner) : Owner(owner) { Owner.m_RunningScripts = true; }
			~ScriptPassScope() { Owner.m_RunningScripts = false; }
		} scriptPassScope(*this);

		UpdateBehaviours(ts);

		// A level without a world (scratch levels, or a failed world create)
		// still runs scripts above — it just has no physics to advance
		if (m_PhysicsWorld)
		{
			// Physics owns the transform of dynamic bodies (RAD-28): nothing
			// pushes ECS transforms into Box2D here — the simulation advances
			// from its own state. ECS→Box2D writes happen only at spawn (the
			// component signals) and through the explicit verbs.
			m_PhysicsWorld->Step(ts);

			// Drain the move events: physics reports what moved, we write only
			// those transforms (playbook §4) — cost scales with activity, not
			// population; sleeping and static bodies produce no events. Must
			// happen inside the fixed step: the NEXT step's scripts read these
			// transforms.
			for (const auto& move : m_PhysicsWorld->GetMoveEvents())
			{
				// Nothing destroys entities between Step and this drain, so a
				// miss is a broken invariant, not a content mistake. Scripts
				// CAN destroy entities (RAD-95), but they run BEFORE the step in
				// this same function, and the assert survives deferred
				// destruction (RAD-97) BECAUSE physics teardown stayed eager: a
				// marked entity's body dies inside the mark, so the step that
				// follows cannot report a move for it. Had the body waited for
				// the reap, this would have had to become a guard — and a
				// destroyed bullet would have gone on colliding for the rest of
				// the step. The map lookup is also why the mark erases the entry
				// itself: a pending entity is absent here, not merely dead.
				auto it = m_EntityMap.find(move.EntityId);
				RADIANT_ASSERT(it != m_EntityMap.end(), "Move-event drain: entity missing from map - destroyed between Step and drain?");
				if (it == m_EntityMap.end())
					continue;

				Entity entity = it->second;

				// IsActive gating preserved from the old readback: an inactive
				// entity's body keeps simulating (pre-existing behavior), its
				// ECS transform just stops following
				if (!entity.GetComponent<MetadataComponent>().IsActive)
					continue;

				auto& transform = entity.GetComponent<TransformComponent>();
				transform.Translation.x = move.Position.x;
				transform.Translation.y = move.Position.y;
				transform.Rotation.z = move.Rotation;
			}

			// Contacts AFTER the move drain: handlers ask "where am I?" and
			// must read this step's transforms, not the previous step's. This
			// is also the only place gameplay code runs with the physics world
			// idle, which is what makes destroying entities from a handler safe
			// (RAD-29).
			DispatchContactEvents();
		}

		// 6pm: everything marked during this step is freed here, and nowhere
		// else. Last, so it catches marks made by contact handlers as well as by
		// scripts; and OUTSIDE the physics guard above, because a scratch level
		// runs scripts, can therefore destroy, and would otherwise accumulate
		// corpses nothing ever collects (RAD-97).
		ReapDestroyedEntities();
	}

	// lerp(snapshot, current, alpha) for entities that have a snapshot; entities
	// without one (static, or spawned mid-step) draw their current state — which
	// makes spawns snap instead of smearing in from a stale position
	static glm::mat4 InterpolatedTransform(const TransformComponent& current, const TransformSnapshotComponent* snapshot, float alpha)
	{
		if (!snapshot)
			return current.GetTransform();

		glm::vec3 translation = glm::mix(snapshot->Translation, current.Translation, alpha);
		glm::vec3 rotation = glm::mix(snapshot->Rotation, current.Rotation, alpha);

		return glm::translate(glm::mat4(1.0f), translation)
			* glm::toMat4(glm::quat(rotation))
			* glm::scale(glm::mat4(1.0f), current.Scale);   // scale is not simulated
	}

	void Level::OnRender(float alpha)
	{
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = GetLiveEntitiesWith<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					// The camera is script-driven (movable) — interpolate it too, or
					// the world is smooth while the view judders
					cameraTransform = InterpolatedTransform(transform, m_Registry.try_get<TransformSnapshotComponent>(entity), alpha);
					break;
				}
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			auto view = GetLiveEntitiesWith<MetadataComponent, TransformComponent, SpriteComponent>();
			for (auto entityHandle : view)
			{
				auto [metadata, transform, sprite] = view.get<MetadataComponent, TransformComponent, SpriteComponent>(entityHandle);

				// Skip inactive Entities
				if (!metadata.IsActive) continue;

				glm::mat4 worldTransform = InterpolatedTransform(transform, m_Registry.try_get<TransformSnapshotComponent>(entityHandle), alpha);

				if (sprite.TextureHandle == 0)
				{
					Renderer2D::DrawSprite(worldTransform, sprite.Color);
				}
				else if (Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(sprite.TextureHandle))
				{
					Renderer2D::DrawSprite(worldTransform, texture, sprite.TilingFactor, sprite.Color);
				}
				
				// Debug colliders deliberately draw the UNINTERPOLATED simulation
				// pose (sim truth): they may lead interpolated sprites by up to one
				// step — that gap is real, not a bug
				if (m_ShowPhysicsColliders)
				{
					Entity e = { entityHandle, this };
					if(auto* bc2d = e.TryGetComponent<BoxCollider2DComponent>())
						DebugDrawCollider(transform, *bc2d);
				}
			}

			Renderer2D::EndScene();
		}

	}

	void Level::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize the non-FixedAspectRatio cameras
		auto view = GetLiveEntitiesWith<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Level::FindEntityByName(std::string_view name)
	{
		auto view = GetLiveEntitiesWith<MetadataComponent>();
		for (auto entity : view)
		{
			const MetadataComponent& metadata = view.get<MetadataComponent>(entity);
			if (metadata.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity Level::GetEntityByUUID(UUID uuid)
	{
		// One lookup, not find-then-at: the contact dispatch resolves both
		// participants before every callback, so this is on a per-collision path
		auto it = m_EntityMap.find(uuid);
		if (it != m_EntityMap.end())
			return it->second;

		return {};
	}

	std::unordered_set<AssetHandle> Level::GetAssetList()
	{
		std::unordered_set<AssetHandle> assetList;
		std::unordered_set<AssetHandle> missingAssets; // Debug only

		// SpriteComponent
		{
			auto view = GetLiveEntitiesWith<SpriteComponent>();
			for (auto entity : view)
			{
				const auto& src = m_Registry.get<SpriteComponent>(entity);
				if (src.TextureHandle)
				{
					if (AssetManager::IsAssetHandleValid(src.TextureHandle))
					{
						assetList.insert(src.TextureHandle);
					}
					else
					{
						missingAssets.insert(src.TextureHandle);
					}
				}
			}
		}

		// TextComponent
		{
			auto view = GetLiveEntitiesWith<TextComponent>();
			for (auto entity : view)
			{
				const auto& tc = m_Registry.get<TextComponent>(entity);
				if (tc.FontHandle)
				{
					if (AssetManager::IsAssetHandleValid(tc.FontHandle))
					{
						assetList.insert(tc.FontHandle);
					}
					else
					{
						missingAssets.insert(tc.FontHandle);
					}
				}
			}
		}

		// The count is normal lifecycle info; only actual misses warrant WARN —
		// and each missing handle is named so the broken reference is findable
		RADIANT_INFO("Level: {} assets referenced ({} missing)", assetList.size(), missingAssets.size());
		for (AssetHandle missing : missingAssets)
			RADIANT_WARN("Level: referenced asset {} is missing from the registry", missing);
		return assetList;
	}

	// The signal handlers below only ever run on live levels: scratch levels
	// (initialize == false) never connect them. A null world here means the
	// wiring changed — a programmer error, guarded so Dist recovers.

	void Level::OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		RADIANT_ASSERT(m_PhysicsWorld, "Physics signal fired on a level without a physics world");
		if (!m_PhysicsWorld)
			return;

		Entity e = { entity, this };
		auto& rb2d = e.GetComponent<RigidBody2DComponent>();
		m_PhysicsWorld->CreateBody(e, rb2d);
	}

	void Level::OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		RADIANT_ASSERT(m_PhysicsWorld, "Physics signal fired on a level without a physics world");
		if (!m_PhysicsWorld)
			return;

		Entity e = { entity, this };
		auto& rb2d = e.GetComponent<RigidBody2DComponent>();
		m_PhysicsWorld->DestroyBody(e, rb2d);
	}

	void Level::OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		RADIANT_ASSERT(m_PhysicsWorld, "Physics signal fired on a level without a physics world");
		if (!m_PhysicsWorld)
			return;

		Entity e = { entity, this };
		auto& bc2d = e.GetComponent<BoxCollider2DComponent>();
		m_PhysicsWorld->CreateBoxShape(e, bc2d);
	}

	void Level::OnBoxCollider2DComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		RADIANT_ASSERT(m_PhysicsWorld, "Physics signal fired on a level without a physics world");
		if (!m_PhysicsWorld)
			return;

		Entity e = { entity, this };
		auto& bc2d = e.GetComponent<BoxCollider2DComponent>();
		m_PhysicsWorld->DestroyBoxShape(e, bc2d);
	}

	void Level::SortEntities()
	{
		m_Registry.sort<MetadataComponent>([&](const auto lhs, const auto rhs)
			{
				auto lhsEntity = m_EntityMap.find(lhs.ID);
				auto rhsEntity = m_EntityMap.find(rhs.ID);
				// A metadata ID absent from the map would deref end() below —
				// programmer error (map and registry out of sync)
				RADIANT_ASSERT(lhsEntity != m_EntityMap.end() && rhsEntity != m_EntityMap.end(), "SortEntities: metadata ID missing from entity map");
				// Unmapped sorts last; two unmapped entries must compare equivalent
				// (false both ways) or the comparator breaks strict weak ordering — UB
				if (lhsEntity == m_EntityMap.end() || rhsEntity == m_EntityMap.end())
					return lhsEntity != m_EntityMap.end() && rhsEntity == m_EntityMap.end();
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});
	}

}
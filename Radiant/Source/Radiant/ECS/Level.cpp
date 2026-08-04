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

		// Destroy all entities one by one rather than calling m_Registry.clear()
		// This ensures component on_destroy signals are fired in the correct order.
		// Bodies die here, via the signals, while m_PhysicsWorld is still alive;
		// the Scope then destroys the world itself after this destructor body.
		// Scratch levels hold a null Scope — their destruction touches no other
		// Level's physics (the RAD-27 fix).
		for (auto entity : GetAllEntitiesWith<MetadataComponent>())
		{
			DestroyEntity({ entity, this });
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

	void Level::DestroyEntity(Entity entity)
	{
		// Stale/null handles reach here from gameplay code (e.g. double-destroy);
		// a config-level mistake, not grounds for UB in Dist
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: DestroyEntity called with an invalid entity handle");
			return;
		}

		if (auto* nsc = entity.TryGetComponent<NativeScriptComponent>())
		{
			// Instance is created lazily on first update — it may not exist yet
			if (nsc->Instance)
			{
				nsc->Instance->OnDestroy();
				if (nsc->DestroyScript)
					nsc->DestroyScript(nsc);
				else
					delete nsc->Instance;
			}
		}

		UUID id = entity.GetUUID();

		// Remove components for which there exist on_destroy handlers
		// This ensures that if the handlers rely on other entity components (in particular
		// the MetadataComponent and the TransformComponent), they can still access them.
		// Collider before rigidbody: destroying a body destroys its shapes inside
		// Box2D, so body-first would leave the collider handler a stale ticket.
		entity.RemoveComponentIfExists<BoxCollider2DComponent>();
		entity.RemoveComponentIfExists<RigidBody2DComponent>();

		m_Registry.destroy(entity.m_EntityHandle);
		m_EntityMap.erase(id);

		SortEntities();
	}

	void Level::DestroyEntity(UUID entityID)
	{
		auto it = m_EntityMap.find(entityID);
		if (it == m_EntityMap.end())
			return;

		DestroyEntity(it->second);
	}

	void Level::Teleport(Entity entity, const glm::vec3& translation, float rotationZ, TeleportType teleportType)
	{
		// Stale/null handles reach here from gameplay — a content-level
		// mistake (same contract as DestroyEntity), not grounds for UB
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: Teleport called with an invalid entity handle");
			return;
		}

		auto& transform = entity.GetComponent<TransformComponent>();
		transform.Translation = translation;
		transform.Rotation.z = rotationZ;

		// Reset the render snapshot to the destination: OnRender draws
		// lerp(snapshot, current, alpha), and a stale snapshot would smear the
		// jump across one rendered frame
		m_Registry.emplace_or_replace<TransformSnapshotComponent>(entity.m_EntityHandle, transform.Translation, transform.Rotation);

		// The one legitimate ECS→Box2D transform push (RAD-28). A body-less
		// entity has already been fully moved by the two writes above —
		// teleportType is simply meaningless without a velocity to act on.
		if (m_PhysicsWorld && entity.HasComponent<RigidBody2DComponent>())
			m_PhysicsWorld->Teleport(entity, { translation.x, translation.y }, rotationZ, teleportType);
	}

	void Level::Teleport(Entity entity, const glm::vec3& translation, TeleportType teleportType)
	{
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: Teleport called with an invalid entity handle");
			return;
		}

		Teleport(entity, translation, entity.GetComponent<TransformComponent>().Rotation.z, teleportType);
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
		auto* nsc = entity.TryGetComponent<NativeScriptComponent>();

		// No script, or one not instantiated yet (instances are created lazily
		// on the first fixed update after binding): nothing to notify
		if (!nsc || !nsc->Instance)
			return;

		// Same gate as OnUpdate: an inactive entity's body keeps colliding, its
		// script just stops hearing about it
		if (!entity.GetComponent<MetadataComponent>().IsActive)
			return;

		// Instance is read BEFORE the call and never touched after it — the
		// handler is allowed to destroy this very entity, which deletes the
		// instance whose method is running
		EntityBehaviour* instance = nsc->Instance;
		if (phase == ContactPhase::Begin)
			instance->OnCollisionBegin(other);
		else
			instance->OnCollisionEnd(other);
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
			// Every resolution below happens at the moment of use, never
			// hoisted: a callback for an EARLIER event in this batch may have
			// destroyed either participant. An unresolvable side arrives as
			// UUID(0), which is never in the map, so it needs no special case.
			if (!GetEntityByUUID(contact.EntityA) && !GetEntityByUUID(contact.EntityB))
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

				CollisionEvent collision{ GetEntityByUUID(contact.EntityA), GetEntityByUUID(contact.EntityB), contact.Phase };
				if (!collision.A && !collision.B)
					break; // an earlier callback destroyed both participants

				m_CollisionCallbacks[i].Function(collision);
			}

			// Then each side's script hook, with the other side resolved at the
			// moment of the call
			if (Entity a = GetEntityByUUID(contact.EntityA))
				NotifyScript(a, GetEntityByUUID(contact.EntityB), contact.Phase);

			// Re-resolved rather than reused: A's handler may have destroyed B
			if (Entity b = GetEntityByUUID(contact.EntityB))
				NotifyScript(b, GetEntityByUUID(contact.EntityA), contact.Phase);
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
		// clear() and refill m_ScriptUpdateList while the outer walk holds
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
			for (auto entityHandle : m_Registry.view<RigidBody2DComponent, TransformComponent>())
				snapshot(entityHandle);
			for (auto entityHandle : m_Registry.view<CameraComponent, TransformComponent>())
				snapshot(entityHandle);
			for (auto entityHandle : m_Registry.view<NativeScriptComponent, TransformComponent>())
				snapshot(entityHandle);

			// An entity that STOPPED being movable must lose its snapshot, or
			// OnRender lerps it against that stale pose forever. Collect-then-
			// remove: removing the iterated component mid-iteration is against
			// the rules (Level.h); the vector only allocates on the rare frame
			// a mover component was actually removed.
			std::vector<entt::entity> staleSnapshots;
			for (auto entityHandle : m_Registry.view<TransformSnapshotComponent>())
			{
				if (!m_Registry.any_of<RigidBody2DComponent, CameraComponent, NativeScriptComponent>(entityHandle))
					staleSnapshots.push_back(entityHandle);
			}
			for (auto entityHandle : staleSnapshots)
				m_Registry.remove<TransformSnapshotComponent>(entityHandle);
		}

		// Snapshot the script set before running any of it. Gameplay can reach
		// CreateEntity/DestroyEntity from here (RAD-95), and a live entt view
		// cannot survive that: binding a script to a spawned entity may
		// reallocate the very pool being walked, and destroying an entity
		// swap-and-pops it (playbook §8.8). Walking plain ids instead means a
		// mutation is at worst a stale id, which the checks below detect.
		// Rebuilt per step; RAD-30's side table replaces the rebuild with a
		// maintained list, which is the shape UE's tick registry already has.
		// Scoped like DispatchScope above, and for the same reason: a gameplay
		// callback that throws must not leave the flag set and wedge every
		// later fixed update.
		struct ScriptPassScope
		{
			Level& Owner;
			explicit ScriptPassScope(Level& owner) : Owner(owner) { Owner.m_RunningScripts = true; }
			~ScriptPassScope() { Owner.m_RunningScripts = false; }
		} scriptPassScope(*this);

		{
			auto scripts = m_Registry.view<NativeScriptComponent>();
			m_ScriptUpdateList.clear();
			// Size is known here, so take it — after the first few steps the
			// member has settled and this reserves nothing (playbook §7)
			m_ScriptUpdateList.reserve(scripts.size());
			for (auto entityHandle : scripts)
				m_ScriptUpdateList.push_back(entityHandle);
		}

		for (entt::entity entityHandle : m_ScriptUpdateList)
		{
			// An earlier script in this same pass may have destroyed this one
			if (!m_Registry.valid(entityHandle))
				continue;

			auto* nsc = m_Registry.try_get<NativeScriptComponent>(entityHandle);
			if (!nsc)
				continue;

			if (!nsc->Instance)
			{
				// A component added without Bind<T>() leaves InstantiateScript
				// empty; calling it throws std::bad_function_call
				RADIANT_ASSERT(nsc->InstantiateScript, "NativeScriptComponent has no bound script - missing Bind<T>()?");
				if (!nsc->InstantiateScript)
					continue;

				EntityBehaviour* instance = nsc->InstantiateScript();
				instance->m_Entity = Entity{ entityHandle, this };
				// Stored BEFORE OnCreate runs: a script that destroys its own
				// entity there must be findable by DestroyEntity, or the
				// instance leaks and its OnDestroy never runs
				nsc->Instance = instance;
				instance->OnCreate();

				// OnCreate is gameplay, so `nsc` may no longer be usable: this
				// entity could be gone, and spawning a scripted entity
				// reallocates the pool nsc points into. That is REFERENCE
				// invalidation — a different hazard from the iterator
				// invalidation the snapshot above fixes, and not cured by it.
				// Re-resolve rather than reuse; this is load-bearing, not
				// defensive noise.
				if (!m_Registry.valid(entityHandle))
					continue;

				nsc = m_Registry.try_get<NativeScriptComponent>(entityHandle);
				if (!nsc || !nsc->Instance)
					continue;
			}

			// Straight from the registry: the handle is already in hand, so
			// routing this through the script's own Entity would be a detour
			if (!m_Registry.get<MetadataComponent>(entityHandle).IsActive)
				continue;

			nsc->Instance->OnUpdate(ts);
		}

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
				// CAN destroy entities now (RAD-95), but they run BEFORE the
				// step in this same function — a destroyed entity's body dies
				// with it, so the step never reports a move for it. That
				// ordering is what keeps this an assert rather than a guard.
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
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
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

			auto view = GetAllEntitiesWith<MetadataComponent, TransformComponent, SpriteComponent>();
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
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Level::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<MetadataComponent>();
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
			auto view = m_Registry.view<SpriteComponent>();
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
			auto view = m_Registry.view<TextComponent>();
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
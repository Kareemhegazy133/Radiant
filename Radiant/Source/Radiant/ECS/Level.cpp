#include "Radiant/rdpch.h"
#include "Level.h"

#include "Radiant/Asset/AssetManager.h"

#include "Radiant/Renderer/Renderer2D.h"

#include "Entity.h"
#include "Components.h"
//#include "Radiant/GAS/AbilitySystemComponent.h"
#include "ScriptableEntity.h"

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

	void Level::Teleport(Entity entity, const glm::vec3& translation, float rotationZ)
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

		// The one legitimate ECS→Box2D transform push (RAD-28)
		if (m_PhysicsWorld && entity.HasComponent<RigidBody2DComponent>())
			m_PhysicsWorld->Teleport(entity, { translation.x, translation.y }, rotationZ);
	}

	void Level::Teleport(Entity entity, const glm::vec3& translation)
	{
		if (!entity.IsValid())
		{
			RADIANT_WARN("Level: Teleport called with an invalid entity handle");
			return;
		}

		Teleport(entity, translation, entity.GetComponent<TransformComponent>().Rotation.z);
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

	void Level::OnFixedUpdate(Timestep ts)
	{
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

		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					// A component added without Bind<T>() leaves InstantiateScript
					// empty; calling it throws std::bad_function_call
					RADIANT_ASSERT(nsc.InstantiateScript, "NativeScriptComponent has no bound script - missing Bind<T>()?");
					if (!nsc.InstantiateScript)
						return;

					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}
				auto& metadata = nsc.Instance->m_Entity.GetComponent<MetadataComponent>();
				if (!metadata.IsActive) return;

				nsc.Instance->OnUpdate(ts);
			});

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
				// Nothing destroys entities between Step and this drain
				// (scripts run BEFORE the step, in this same function), so a
				// miss is a broken invariant, not a content mistake
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
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

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
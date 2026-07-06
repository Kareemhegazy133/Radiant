#include "Radiant/rdpch.h"
#include "Level.h"

#include "Radiant/Asset/AssetManager.h"

#include "Radiant/Renderer/Renderer2D.h"

#include "Entity.h"
#include "Components.h"
//#include "Radiant/GAS/AbilitySystemComponent.h"
#include "ScriptableEntity.h"

#include "Radiant/Physics/Physics2D.h"

namespace Radiant {

	Level::Level(const std::string& name, bool initialize)
		: m_Name(name)
	{
		if (!initialize) return;

		RADIANT_TRACE("Level Constructor");
		Physics2D::Init(this);

		// entt construct/destroy signals
		// Components with signals registered here should be explicitly removed from the entity
		// in DestroyEntity() before the entity is itself destroyed. This ensures that the on_destroy()
		// handlers will be called before the entity is destroyed (in particular before the entity's
		// MetadataComponent and TransformComponent are destroyed)
		m_Registry.on_construct<RigidBody2DComponent>().connect<&Level::OnRigidBody2DComponentConstruct>(this);
		m_Registry.on_destroy<RigidBody2DComponent>().connect<&Level::OnRigidBody2DComponentDestroy>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Level::OnBoxCollider2DComponentConstruct>(this);

		RADIANT_TRACE("Level Constructed: {0}", (void*)this);
	}

	Level::~Level()
	{
		RADIANT_TRACE("Level Destructor");

		// Destroy all entities one by one rather than calling m_Registry.clear()
		// This ensures component on_destroy signals are fired in the correct order.
		for (auto entity : GetAllEntitiesWith<MetadataComponent>())
		{
			DestroyEntity({ entity, this });
		}

		// Runs even for levels constructed with initialize == false — destroying a
		// scratch level tears down the live level's shared physics world (RAD-27)
		Physics2D::Shutdown();

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

		auto view = GetAllEntitiesWith<MetadataComponent, RigidBody2DComponent>();
		for (auto entityHandle : view)
		{
			auto [metadata, rb2d] = view.get<
				MetadataComponent, RigidBody2DComponent>(entityHandle);

			// Skip inactive Entities
			if (!metadata.IsActive) continue;

			// Submit Transforms/Colliders of all entities for physics
			Entity entity = { entityHandle, this };
			Physics2D::SubmitEntitiesTransforms(entity);
		}

		// Apply Physics
		Physics2D::OnUpdate(ts);

		// Readback: copy stepped body transforms into the ECS as a dedicated
		// post-step pass (playbook §4). Runs for every active body — unlike the
		// old in-render readback, which skipped bodies without sprites and did
		// nothing at all without a primary camera. Must happen inside the fixed
		// step: the NEXT step's scripts read these transforms.
		for (auto entityHandle : view)
		{
			auto [metadata, rb2d] = view.get<MetadataComponent, RigidBody2DComponent>(entityHandle);
			if (!metadata.IsActive) continue;

			Entity entity = { entityHandle, this };
			Physics2D::UpdateEntitiesTransforms(entity);
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
						Physics2D::DebugDraw(transform, *bc2d);
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

	void Level::OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		Entity e = { entity, this };
		auto& rb2d = e.GetComponent<RigidBody2DComponent>();
		Physics2D::CreatePhysicsBody(e, rb2d);
	}

	void Level::OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		Entity e = { entity, this };
		auto& rb2d = e.GetComponent<RigidBody2DComponent>();
		Physics2D::DestroyPhysicsBody(e, rb2d);
	}

	void Level::OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		Entity e = { entity, this };
		auto& bc2d = e.GetComponent<BoxCollider2DComponent>();
		Physics2D::CreateBoxColliderFixture(e, bc2d);
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
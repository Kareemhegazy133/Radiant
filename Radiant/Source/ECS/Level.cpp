#include "rdpch.h"
#include "Level.h"

#include "Asset/AssetManager.h"

#include "Renderer/Renderer2D.h"

#include "Entity.h"
#include "Components.h"
//#include "GAS/AbilitySystemComponent.h"
#include "ScriptableEntity.h"

#include "Physics/Physics2D.h"

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
		metadata.Type = typeid(Entity);
		metadata.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;

		return entity;
	}

	void Level::DestroyEntity(Entity entity)
	{
		if (auto* nsc = entity.TryGetComponent<NativeScriptComponent>())
		{
			nsc->Instance->OnDestroy();
		}

		UUID id = entity.GetUUID();

		// Remove components for which there exist on_destroy handlers
		// This ensures that if the handlers rely on other entity components (in particular
		// the MetadataComponent and the TransformComponent), they can still access them.
		auto name = entity.Name();
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

	void Level::OnUpdate(Timestep ts)
	{
		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
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
	}

	void Level::OnRender()
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
					cameraTransform = transform.GetTransform();
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

				// Update Transforms/Colliders of all entities from physics
				Entity entity = { entityHandle, this };
				Physics2D::UpdateEntitiesTransforms(entity);

				Renderer2D::DrawSprite(transform.GetTransform(), sprite);

				// For Debugging Purposes
				if (m_ShowPhysicsColliders)
				{
					Entity e = { entity, this };
					if(auto* bc2d = e.TryGetComponent<BoxCollider2DComponent>())
						Physics2D::DebugDraw(transform, *bc2d);
				}
			}
			
			// Draw text
			{
				auto view = m_Registry.view<TransformComponent, TextComponent>();
				for (auto entity : view)
				{
					auto [transform, text] = view.get<TransformComponent, TextComponent>(entity);
					auto font = Font::GetFontAssetForTextComponent(text);
					Renderer2D::DrawString(text.TextString, font, transform.GetTransform(), text.Color, text.LineSpacing, text.Kerning);
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
				if (src.Texture)
				{
					if (AssetManager::IsAssetHandleValid(src.Texture))
					{
						assetList.insert(src.Texture);
					}
					else
					{
						missingAssets.insert(src.Texture);
					}
				}
			}
		}

		RADIANT_WARN("{} assets ({} missing)", assetList.size(), missingAssets.size());
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
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});
	}

}
#include "Enginepch.h"

#include "Level.h"
#include "ScriptableEntity.h"
#include "Components.h"
#include "GAS/AbilitySystemComponent.h"
#include "Entity.h"

namespace Engine {

	Level::Level()
		: m_RenderWindow(GameApplication::GetRenderWindow()), m_Physics(this)
	{
		std::cout << "Level Constructed: " << this << std::endl;
	}

	Level::~Level()
	{
		std::cout << "Level Destroyed: " << this << std::endl;
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
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
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
		
		auto view = GetAllEntitiesWith<MetadataComponent, TransformComponent, SpriteComponent, BoxCollider2DComponent>();
		for (auto entityHandle : view)
		{
			auto [metadata, transform, sprite, bc2d] = view.get<
				MetadataComponent, TransformComponent, SpriteComponent, BoxCollider2DComponent>(entityHandle);

			// Skip inactive Entities
			if (!metadata.IsActive) continue;

			// Update Physics Colliders for all entities
			m_Physics.UpdateBoxColliderFixture(bc2d, transform, sprite);
		}

		// Apply Physics
		m_Physics.OnUpdate(ts);
	}

	void Level::OnRender()
	{
		// Update the transform after applying physics for sprite rendering
		auto view = GetAllEntitiesWith<MetadataComponent, TransformComponent, SpriteComponent>();
		for (auto entity : view)
		{
			auto [metadata, transform, sprite] = view.get<MetadataComponent, TransformComponent, SpriteComponent>(entity);

			// Skip inactive Entities
			if (!metadata.IsActive) continue;

			// Apply the transform to the sprite
			sprite.SetPosition(transform.GetPosition());
			sprite.SetRotation(transform.GetRotation());
			sprite.SetScale(
				{ transform.GetScale().x * sprite.GetScale().x,
				transform.GetScale().y * sprite.GetScale().y }
			);

			// Draw the sprite to the render window
			m_RenderWindow->draw(sprite);
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

	template<typename T>
	void Level::OnComponentAdded(Entity entity, T& component)
	{
		//static_assert(false);
	}

	template<typename T>
	void Level::OnComponentRemoved(Entity entity, T& component)
	{
	}

	template<>
	void Level::OnComponentAdded<MetadataComponent>(Entity entity, MetadataComponent& component)
	{
	}

	template<>
	void Level::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Level::OnComponentAdded<SpriteComponent>(Entity entity, SpriteComponent& component)
	{
	}

	template<>
	void Level::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent& component)
	{
		if (!entity.HasComponent<SpriteComponent>())
		{
			entity.AddComponent<SpriteComponent>();
		}
		component.Sprite = &entity.GetComponent<SpriteComponent>();
	}

	template<>
	void Level::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
		m_Physics.CreatePhysicsBody(entity, component);
	}

	template<>
	void Level::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
		m_Physics.CreateBoxColliderFixture(entity, component);
	}

	template<>
	void Level::OnComponentRemoved<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
		m_Physics.DestroyBoxColliderFixture(entity, component);
	}

	template<>
	void Level::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Level::OnComponentAdded<AbilitySystemComponent>(Entity entity, AbilitySystemComponent& component)
	{
	}
}
#include "Enginepch.h"

#include "Core/GameApplication.h"

#include "World.h"
#include "Components.h"
#include "Entities/Entity.h"

namespace Engine {

	World* World::s_Instance = nullptr;

	World::World()
		: m_RenderWindow(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow()))
	{
		ENGINE_ASSERT(!s_Instance, "World already exists!");
		s_Instance = this;

	}

	World::~World()
	{

	}

	Entity World::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };

		entity.AddComponent<TransformComponent>();
		auto& metadata = entity.AddComponent<MetadataComponent>();
		metadata.Tag = name.empty() ? "Entity" : name;

		return entity;
	}

	void World::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void World::OnUpdate(Timestep ts)
	{
		// Update Physics Bodies and Colliders of all entities
		auto view = m_Registry.view<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>();
		for (auto entity : view)
		{
			auto& [metadata, rb2d, transform, sprite] = view.get<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>(entity);

			// Skip inactive Entities
			if (!metadata.IsActive) continue;

			Entity entity(entity, this);
			entity.OnUpdate(ts);
			
			auto& bc2d = m_Registry.get<BoxCollider2DComponent>(entity);
			m_Physics.UpdateBoxColliderFixture(bc2d, transform, sprite);

			// Update the sprite's animation
			sprite.update(ts);
		}

		// Apply Physics
		m_Physics.OnUpdate(ts);
	}

	void World::OnRender()
	{
		// Update the transform after applying physics for sprite rendering
		auto view = m_Registry.view<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>();
		for (auto entity : view)
		{
			auto& [metadata, rb2d, transform, sprite] = view.get<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>(entity);

			// Skip inactive Entities
			if (!metadata.IsActive) continue;

			// Apply the transform to the sprite
			sprite.setPosition(transform.getPosition());
			sprite.setRotation(transform.getRotation());
			sprite.setScale(transform.getScale());

			// Draw the sprite to the render window
			m_RenderWindow->draw(sprite);
		}
	}

	template<typename T>
	void World::OnComponentAdded(Entity entity, T& component)
	{
		//static_assert(false);
	}

	template<>
	void World::OnComponentAdded<MetadataComponent>(Entity entity, MetadataComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<SpriteComponent>(Entity entity, SpriteComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent& component)
	{
		auto& spriteComponent = entity.GetComponent<SpriteComponent>();
		spriteComponent.Animation = &component;
	}

	template<>
	void World::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
		m_Physics.CreatePhysicsBody(entity, component);
	}

	template<>
	void World::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
		m_Physics.CreateBoxColliderFixture(entity, component);
	}

	template<>
	void World::OnComponentAdded<AttributesComponent>(Entity entity, AttributesComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<AbilitiesComponent>(Entity entity, AbilitiesComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<CharacterComponent>(Entity entity, CharacterComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<AbilityComponent>(Entity entity, AbilityComponent& component)
	{
	}
}
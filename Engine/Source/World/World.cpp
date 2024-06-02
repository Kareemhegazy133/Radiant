#include "Enginepch.h"

#include "World.h"
#include "Components.h"
#include "Entity.h"

namespace Engine {

	World* World::s_Instance = nullptr;

	World::World(sf::RenderWindow* renderWindow)
		: m_RenderWindow(renderWindow)
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
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		return entity;
	}

	void World::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void World::OnUpdate(Timestep ts)
	{
		// Update Physics Bodies and Colliders of all entities
		auto view = m_Registry.view<Rigidbody2DComponent, TransformComponent, SpriteComponent>();
		for (auto entity : view)
		{
			auto& [rb2d, transform, sprite] = view.get<Rigidbody2DComponent, TransformComponent, SpriteComponent>(entity);
			m_Physics.UpdatePhysicsBody(rb2d, transform);
			
			auto& bc2d = m_Registry.get<BoxCollider2DComponent>(entity);
			m_Physics.UpdateBoxColliderFixture(bc2d, transform, sprite);
		}

		// Apply Physics
		m_Physics.OnUpdate(ts);

		// Update the transform after applying physics for sprite rendering
		for (auto entity : view)
		{
			auto& [rb2d, transform, sprite] = view.get<Rigidbody2DComponent, TransformComponent, SpriteComponent>(entity);
			m_Physics.UpdateEntityTransform(rb2d, transform);

			// Update the sprite's animation
			sprite.update(ts);

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
		static_assert(false);
	}

	template<>
	void World::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
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

}
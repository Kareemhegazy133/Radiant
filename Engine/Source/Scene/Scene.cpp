#include "Enginepch.h"

#include "Scene.h"
#include "Components.h"
#include "Entity.h"

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"

#include "Physics/Physics2D.h"

namespace Engine {
	
	Scene::Scene(sf::RenderWindow* renderWindow)
		: m_RenderWindow(renderWindow)
	{
		m_PhysicsWorld = new b2World({ 0.0f, 9.8f });
	}

	Scene::~Scene()
	{
		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };

		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdate(Timestep ts)
	{
		// Update Physics Bodies and Colliders of all entities
		auto view = m_Registry.view<Rigidbody2DComponent, TransformComponent, SpriteComponent>();
		for (auto entity : view)
		{
			auto& [rb2d, transform, sprite] = view.get<Rigidbody2DComponent, TransformComponent, SpriteComponent>(entity);
			UpdatePhysicsBody(rb2d, transform);
			
			auto& bc2d = m_Registry.get<BoxCollider2DComponent>(entity);
			UpdateBoxColliderFixture(bc2d, transform, sprite);
		}

		// Apply Physics
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 2;
		m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);

		// Update the transform after applying physics for sprite rendering
		for (auto entity : view)
		{
			auto& [rb2d, transform, sprite] = view.get<Rigidbody2DComponent, TransformComponent, SpriteComponent>(entity);
			b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
			const auto& position = body->GetPosition();
			transform.setPosition(position.x, position.y);
			transform.setRotation(RAD_TO_DEG(body->GetAngle()));

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
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
		
	}

	template<>
	void Scene::OnComponentAdded<SpriteComponent>(Entity entity, SpriteComponent& component)
	{
		
	}

	template<>
	void Scene::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent& component)
	{
		auto& spriteComponent = entity.GetComponent<SpriteComponent>();
		spriteComponent.Animation = &component;
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
		CreatePhysicsBody(m_PhysicsWorld, entity, component);
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
		CreateBoxColliderFixture(entity, component);
	}

}
#include "Enginepch.h"

#include "Core/GameApplication.h"

#include "World.h"
#include "Components.h"
#include "GameObject.h"

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

	GameObject World::CreateGameObject(const std::string& name)
	{
		GameObject gameObject = { m_Registry.create(), this };

		gameObject.AddComponent<TransformComponent>();
		auto& metadata = gameObject.AddComponent<MetadataComponent>();
		metadata.Tag = name.empty() ? "GameObject" : name;

		return gameObject;
	}

	void World::DestroyGameObject(GameObject gameObject)
	{
		m_Registry.destroy(gameObject);
	}

	void World::OnUpdate(Timestep ts)
	{
		// Update Physics Bodies and Colliders of all gameplay entities
		auto view = m_Registry.view<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>();
		for (auto gameObject : view)
		{
			auto& [metadata, rb2d, transform, sprite] = view.get<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>(gameObject);

			// Skip inactive Gameplay Entities
			if (!metadata.IsActive) continue;

			GameObject gameObject(gameObject, this);
			gameObject.OnUpdate(ts);
			
			auto& bc2d = m_Registry.get<BoxCollider2DComponent>(gameObject);
			m_Physics.UpdateBoxColliderFixture(bc2d, transform, sprite);

			// Update the sprite's animation
			sprite.Update(ts);
		}

		// Apply Physics
		m_Physics.OnUpdate(ts);
	}

	void World::OnRender()
	{
		// Update the transform after applying physics for sprite rendering
		auto view = m_Registry.view<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>();
		for (auto gameObject : view)
		{
			auto& [metadata, rb2d, transform, sprite] = view.get<MetadataComponent, Rigidbody2DComponent, TransformComponent, SpriteComponent>(gameObject);

			// Skip inactive Entities
			if (!metadata.IsActive) continue;

			// Apply the transform to the sprite
			sprite.SetPosition(transform.GetPosition());
			sprite.SetRotation(transform.GetRotation());
			sprite.SetScale(transform.GetScale());

			// Draw the sprite to the render window
			m_RenderWindow->draw(sprite);
		}
	}

	template<typename T>
	void World::OnComponentAdded(GameObject gameObject, T& component)
	{
		//static_assert(false);
	}

	template<>
	void World::OnComponentAdded<MetadataComponent>(GameObject gameObject, MetadataComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<TransformComponent>(GameObject gameObject, TransformComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<SpriteComponent>(GameObject gameObject, SpriteComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<AnimationComponent>(GameObject gameObject, AnimationComponent& component)
	{
		auto& spriteComponent = gameObject.GetComponent<SpriteComponent>();
		spriteComponent.Animation = &component;
	}

	template<>
	void World::OnComponentAdded<Rigidbody2DComponent>(GameObject gameObject, Rigidbody2DComponent& component)
	{
		m_Physics.CreatePhysicsBody(gameObject, component);
	}

	template<>
	void World::OnComponentAdded<BoxCollider2DComponent>(GameObject gameObject, BoxCollider2DComponent& component)
	{
		m_Physics.CreateBoxColliderFixture(gameObject, component);
	}

	template<>
	void World::OnComponentAdded<AttributesComponent>(GameObject gameObject, AttributesComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<AbilitiesComponent>(GameObject gameObject, AbilitiesComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<CharacterComponent>(GameObject gameObject, CharacterComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<AbilityComponent>(GameObject gameObject, AbilityComponent& component)
	{
	}

	template<>
	void World::OnComponentAdded<ButtonComponent>(GameObject gameObject, ButtonComponent& component)
	{
	}
}
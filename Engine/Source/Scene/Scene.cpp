#include "Enginepch.h"

#include "Core/GameApplication.h"

#include "Scene.h"
#include "Components.h"
#include "GameObject.h"

namespace Engine {

	Scene* Scene::s_Instance = nullptr;

	Scene::Scene()
		: m_RenderWindow(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow()))
	{
		ENGINE_ASSERT(!s_Instance, "Scene already exists!");
		s_Instance = this;

	}

	Scene::~Scene()
	{

	}

	GameObject Scene::CreateGameObject(const std::string& name)
	{
		GameObject gameObject = { m_Registry.create(), this };

		gameObject.AddComponent<TransformComponent>();
		auto& metadata = gameObject.AddComponent<MetadataComponent>();
		metadata.Tag = name.empty() ? "GameObject" : name;

		return gameObject;
	}

	void Scene::DestroyGameObject(GameObject gameObject)
	{
		m_Registry.destroy(gameObject);
	}

	template<typename T>
	void Scene::OnComponentAdded(GameObject gameObject, T& component)
	{
		//static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<MetadataComponent>(GameObject gameObject, MetadataComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(GameObject gameObject, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteComponent>(GameObject gameObject, SpriteComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<AnimationComponent>(GameObject gameObject, AnimationComponent& component)
	{
		auto& spriteComponent = gameObject.GetComponent<SpriteComponent>();
		spriteComponent.Animation = &component;
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(GameObject gameObject, Rigidbody2DComponent& component)
	{
		m_Physics.CreatePhysicsBody(gameObject, component);
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(GameObject gameObject, BoxCollider2DComponent& component)
	{
		m_Physics.CreateBoxColliderFixture(gameObject, component);
	}

	template<>
	void Scene::OnComponentAdded<AttributesComponent>(GameObject gameObject, AttributesComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<AbilitiesComponent>(GameObject gameObject, AbilitiesComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CharacterComponent>(GameObject gameObject, CharacterComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<AbilityComponent>(GameObject gameObject, AbilityComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<ButtonComponent>(GameObject gameObject, ButtonComponent& component)
	{
	}
}
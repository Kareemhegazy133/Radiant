#include "Enginepch.h"

#include "Physics/Physics2D.h"

#include "Scene.h"
#include "Components.h"
#include "GameObject.h"

namespace Engine {

	Scene* Scene::s_Instance = nullptr;

	Scene::Scene()
	{
		/*ENGINE_ASSERT(!s_Instance, "Scene already exists!");
		s_Instance = this;*/
		if (!s_Instance)
		{
			s_Instance = this;
		}
		std::cout << "Scene Instance: " << s_Instance << std::endl;
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
		Physics2D::Get().CreatePhysicsBody(gameObject, component);
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(GameObject gameObject, BoxCollider2DComponent& component)
	{
		Physics2D::Get().CreateBoxColliderFixture(gameObject, component);
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
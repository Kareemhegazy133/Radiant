#pragma once

#include "entt.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "Core/Timestep.h"

class b2World;

namespace Engine {

	class Entity;

	class Scene
	{
	public:
		Scene(sf::RenderWindow* renderWindow);
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);
		void OnUpdate(Timestep ts);
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		sf::RenderWindow* m_RenderWindow;
		entt::registry m_Registry;

		b2World* m_PhysicsWorld = nullptr;

		friend class Entity;
	};

}
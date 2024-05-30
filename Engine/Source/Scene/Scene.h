#pragma once

#include "entt.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "Core/Timestep.h"

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
		friend class Entity;
	};
}
#pragma once

#include "entt.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "Core/Timestep.h"

class b2World;

namespace Engine {

	class Entity;

	class World
	{
	public:
		World(sf::RenderWindow* renderWindow);
		~World();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);
		virtual void OnUpdate(Timestep ts);

		inline static World& GetWorld() { return *s_Instance; }

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	protected:
		sf::RenderWindow* m_RenderWindow;

	private:
		static World* s_Instance;
		entt::registry m_Registry;

		b2World* m_PhysicsWorld = nullptr;

		friend class Entity;
	};

}
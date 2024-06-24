#pragma once

#include "entt.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "Core/Timestep.h"

#include "Physics/Physics2D.h"

namespace Engine {

	class GameObject;

	class World
	{
	public:
		World();
		~World();

		GameObject CreateGameObject(const std::string& name = std::string());
		void DestroyGameObject(GameObject gameObject);

		virtual void OnUpdate(Timestep ts);
		void OnRender();

		inline static World& GetWorld() { return *s_Instance; }

	private:
		template<typename T>
		void OnComponentAdded(GameObject gameObject, T& component);

	protected:
		sf::RenderWindow* m_RenderWindow;

	private:
		static World* s_Instance;
		entt::registry m_Registry;

		Physics2D m_Physics;

		friend class GameObject;
	};

}
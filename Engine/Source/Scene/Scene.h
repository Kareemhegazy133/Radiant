#pragma once

#include "entt.hpp"
#include "SFML/Graphics/RenderWindow.hpp"

namespace Engine {

	class GameObject;

	class Scene
	{
	public:
		Scene();
		~Scene();

		GameObject CreateGameObject(const std::string& name = std::string());
		void DestroyGameObject(GameObject gameObject);

		inline static Scene& GetScene() { return *s_Instance; }

	protected:
		template<typename... Components>
		auto GetView()
		{
			return m_Registry.view<Components...>();
		}

	private:
		template<typename T>
		void OnComponentAdded(GameObject gameObject, T& component);

	private:
		static Scene* s_Instance;
		entt::registry m_Registry;

		friend class GameObject;
	};

}
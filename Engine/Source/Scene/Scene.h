#pragma once

#include "entt.hpp"
#include "SFML/Graphics/RenderWindow.hpp"

#include "Core/UUID.h"

namespace Engine {

	class GameObject;

	class Scene
	{
	public:
		Scene();
		virtual ~Scene();

		GameObject CreateGameObject(const std::string& name = std::string());
		GameObject CreateGameObjectWithUUID(UUID uuid, const std::string& name = std::string());

		void DestroyGameObject(GameObject gameObject);

		inline static Scene& GetScene() { return *s_Instance; }

	protected:
		template<typename... Components>
		auto GetAllGameObjectsWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		template<typename T>
		void OnComponentAdded(GameObject gameObject, T& component);

		template<typename T>
		void OnComponentRemoved(GameObject gameObject, T& component);

	private:
		static Scene* s_Instance;
		entt::registry m_Registry;

		std::unordered_map<UUID, entt::entity> m_GameObjectMap;

		friend class GameObject;
		friend class SceneSerializer;
	};

}
#pragma once

#include <entt.hpp>

#include "Core/GameApplication.h"
#include "Core/UUID.h"

#include "Physics/Physics2D.h"

namespace Engine {

	class Entity;

	class Level
	{
	public:
		Level();
		~Level();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());

		void DestroyEntity(Entity entity);

		void OnUpdate(Timestep ts);
		void OnRender();

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

	protected:
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		template<typename T>
		void OnComponentRemoved(Entity entity, T& component);

	protected:
		sf::RenderWindow* m_RenderWindow;

	private:
		Physics2D m_Physics;
		entt::registry m_Registry;

		std::unordered_map<UUID, entt::entity> m_EntityMap;

		friend class Entity;
		friend class LevelSerializer;
	};

}
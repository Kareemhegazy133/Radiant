#pragma once

#include <entt.hpp>

#include "Core/GameApplication.h"
#include "Core/UUID.h"

#include "Entity.h"

namespace Radiant {

	class Level
	{
	public:
		Level();
		~Level();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());

		void DestroyEntity(Entity entity);
		void DestroyEntity(UUID entityID);

		void OnUpdate(Timestep ts);
		void OnRender();

		void OnViewportResize(uint32_t width, uint32_t height);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

	protected:
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		void OnRigidBody2DComponentConstruct(entt::registry& registry, entt::entity entity);
		void OnRigidBody2DComponentDestroy(entt::registry& registry, entt::entity entity);
		void OnBoxCollider2DComponentConstruct(entt::registry& registry, entt::entity entity);

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::unordered_map<UUID, Entity> m_EntityMap;

		// For Debugging Purposes
		bool m_ShowPhysicsColliders = true;

		friend class Entity;
		friend class Physics2D;
		friend class LevelSerializer;
	};

}

#include "EntityTemplates.h"
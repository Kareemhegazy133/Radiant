#pragma once

#include <entt.hpp>

#include "Level.h"
#include "ECS/Components.h"

namespace Engine {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Level* level);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			ENGINE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_Level->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Level->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Level->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Level->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			ENGINE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Level->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		T* TryGetComponent()
		{
			return m_Level->m_Registry.try_get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Level->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			T& component = GetComponent<T>();
			m_Level->OnComponentRemoved<T>(*this, component);
			m_Level->m_Registry.remove<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		UUID GetUUID() { return GetComponent<MetadataComponent>().ID; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Level == other.m_Level;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Level* m_Level = nullptr;
	};
}
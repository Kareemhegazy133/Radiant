#pragma once

#include "entt.hpp"
#include "Scene.h"

namespace Engine {
	// TODO: Fix these Asserts

	class GameObject
	{
	public:
		GameObject() = default;
		GameObject(entt::entity handle, Scene* scene);
		GameObject(const GameObject& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			//ENGINE_ASSERT(!HasComponent<T>(), "GameObject already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_GameObjectHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			//ENGINE_ASSERT(HasComponent<T>(), "GameObject does not have component!");
			return m_Scene->m_Registry.get<T>(m_GameObjectHandle);
		}

		template<typename T>
		T* TryGetComponent()
		{
			return m_Scene->m_Registry.try_get<T>(m_GameObjectHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_GameObjectHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			ENGINE_ASSERT(HasComponent<T>(), "GameObject does not have component!");
			m_Scene->m_Registry.remove<T>(m_GameObjectHandle);
		}

		operator bool() const { return m_GameObjectHandle != entt::null; }
		operator entt::entity() const { return m_GameObjectHandle; }
		operator uint32_t() const { return (uint32_t)m_GameObjectHandle; }

		bool operator==(const GameObject& other) const
		{
			return m_GameObjectHandle == other.m_GameObjectHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const GameObject& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_GameObjectHandle{ entt::null };
		Scene* m_Scene = nullptr;
	};
}
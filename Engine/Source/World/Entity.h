#pragma once

#include "entt.hpp"
#include "World.h"
#include "Components.h"

namespace Engine {
	
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, World* world);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			ENGINE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_World->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_World->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			ENGINE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_World->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_World->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			ENGINE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_World->m_Registry.remove<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_World == other.m_World;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}
	protected:
		virtual void OnCollisionBegin(Entity& other);
		virtual void OnCollisionEnd(Entity& other);

		void SetupAnimation(const std::string& animationName, int frameCount, int frameWidth, int frameHeight,
			int frameWidthPadding, int frameHeightPadding, float frameDuration);

	protected:
		uint8_t m_FrameCount;
		uint16_t m_FrameWidth;
		uint16_t m_FrameHeight;
		uint8_t m_FrameWidthPadding = 0;
		uint8_t m_FrameHeightPadding = 0;

	private:
		entt::entity m_EntityHandle{ entt::null };
		World* m_World = nullptr;
	};
}
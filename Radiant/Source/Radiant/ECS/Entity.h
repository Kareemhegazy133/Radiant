#pragma once

#include <entt.hpp>

#include "Components.h"

namespace Radiant {

	class Level;

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Level* level)
			: m_EntityHandle(handle), m_Level(level) {}
		Entity(const Entity& other) = default;

		~Entity() = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args);

		template<typename T>
		T& GetComponent();

		template<typename T>
		const T& GetComponent() const;

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		T* TryGetComponent();

		// returns nullptr if entity does not have the requested component type
		template<typename T>
		const T* TryGetComponent() const;

		template<typename... T>
		bool HasComponent();

		template<typename... T>
		bool HasComponent() const;

		template<typename T>
		void RemoveComponent();

		template<typename T>
		void RemoveComponentIfExists();

		std::string& Name() { return HasComponent<MetadataComponent>() ? GetComponent<MetadataComponent>().Tag : NoName; }
		const std::string& Name() const { return HasComponent<MetadataComponent>() ? GetComponent<MetadataComponent>().Tag : NoName; }

		operator bool() const;
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		UUID GetUUID() { return GetComponent<MetadataComponent>().ID; }

		bool IsValid() const;

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

		inline static std::string NoName = "Unnamed";

		friend class Level;
	};
}
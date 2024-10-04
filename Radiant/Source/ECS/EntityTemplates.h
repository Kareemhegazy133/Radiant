#pragma once

namespace Radiant {

	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		RADIANT_ASSERT(!HasComponent<T>(), "Entity already has component!");
		T& component = m_Level->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T, typename... Args>
	T& Entity::AddOrReplaceComponent(Args&&... args)
	{
		T& component = m_Level->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T>
	T& Entity::GetComponent()
	{
		RADIANT_ASSERT(HasComponent<T>(), "Entity does not have component!");
		return m_Level->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	const T& Entity::GetComponent() const
	{
		RADIANT_ASSERT(HasComponent<T>(), "Entity does not have component!");
		return m_Level->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	T* Entity::TryGetComponent()
	{
		RADIANT_ASSERT(IsValid());
		return m_Level->m_Registry.try_get<T>(m_EntityHandle);
	}

	template<typename T>
	const T* Entity::TryGetComponent() const
	{
		RADIANT_ASSERT(IsValid());
		return m_Level->m_Registry.try_get<T>(m_EntityHandle);
	}

	template<typename... T>
	bool Entity::HasComponent()
	{
		RADIANT_ASSERT(IsValid());
		return m_Level->m_Registry.all_of<T...>(m_EntityHandle);
	}

	template<typename... T>
	bool Entity::HasComponent() const
	{
		RADIANT_ASSERT(IsValid());
		return m_Level->m_Registry.all_of<T...>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		RADIANT_ASSERT(HasComponent<T>(), "Entity does not have component!");
		m_Level->m_Registry.remove<T>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponentIfExists()
	{
		RADIANT_ASSERT(IsValid());
		if(HasComponent<T>())
			RemoveComponent<T>();
	}

}
#pragma once

#include "Entity.h"

namespace Radiant {

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		bool operator==(const ScriptableEntity& other) const
		{
			return m_Entity == other.m_Entity;
		}

		bool operator!=(const ScriptableEntity& other) const
		{
			return !(*this == other);
		}

		bool operator==(const Entity& other) const
		{
			return m_Entity == other;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	protected:
		virtual void OnCreate() { RADIANT_INFO("Scriptable OnCreate"); }
		virtual void OnUpdate(Timestep ts) { RADIANT_INFO("Scriptable OnUpdate"); }
		virtual void OnDestroy() { RADIANT_INFO("Scriptable OnDestroy"); }

	private:
		Entity m_Entity;
		friend class Level;
	};

}
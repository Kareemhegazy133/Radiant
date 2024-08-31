#include "Enginepch.h"

#include "Entity.h"

namespace Engine {

	Entity::Entity(entt::entity handle, Level* level)
		: m_EntityHandle(handle), m_Level(level)
	{
		
	}
	
}
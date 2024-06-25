#include "Enginepch.h"
#include "GameObject.h"

namespace Engine {

	GameObject::GameObject(entt::entity handle, Scene* world)
		: m_GameObjectHandle(handle), m_Scene(world)
	{
		
	}
}
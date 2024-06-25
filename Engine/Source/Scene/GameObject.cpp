#include "Enginepch.h"
#include "GameObject.h"

namespace Engine {

	GameObject::GameObject(entt::entity handle, Scene* scene)
		: m_GameObjectHandle(handle), m_Scene(scene)
	{
		
	}
}
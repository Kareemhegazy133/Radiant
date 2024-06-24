#include "Enginepch.h"
#include "GameObject.h"

namespace Engine {

	GameObject::GameObject(entt::entity handle, World* world)
		: m_GameObjectHandle(handle), m_World(world)
	{
		
	}

	void GameObject::OnUpdate(Timestep ts)
	{
		if (GetComponent<MetadataComponent>().OnUpdate)
		{
			GetComponent<MetadataComponent>().OnUpdate(ts);
		}
	}
}
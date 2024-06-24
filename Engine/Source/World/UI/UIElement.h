#pragma once

#include "World/GameObject.h"

namespace Engine {

	class UIElement : public GameObject
	{
	public:
		UIElement(entt::entity handle, World* world)
			: GameObject(handle, world) {}
		UIElement(const UIElement& other) = default;

	};
}
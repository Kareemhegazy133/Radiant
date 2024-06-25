#pragma once

#include "Scene/GameObject.h"

namespace Engine {

	class UIElement : public GameObject
	{
	public:
		UIElement(entt::entity handle, Scene* world)
			: GameObject(handle, world) {}
		UIElement(const UIElement& other) = default;

	};
}
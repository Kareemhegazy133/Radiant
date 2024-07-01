#pragma once

#include "Scene/GameObject.h"

namespace Engine {

	class UIElement : public GameObject
	{
	public:
		UIElement(entt::entity handle, Scene* scene)
			: GameObject(handle, scene) {}
		UIElement(const UIElement& other) = default;

		virtual void OnUpdate(Timestep ts);

	};
}
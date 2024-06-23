#pragma once

#include "World/Entities/Entity.h"

namespace Engine {

	class UIEntity : public Entity
	{
	public:
		UIEntity(entt::entity handle, World* world)
			: Entity(handle, world) {}
		UIEntity(const UIEntity& other) = default;

	};
}
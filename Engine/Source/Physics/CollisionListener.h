#pragma once

#include "box2d/b2_world_callbacks.h"

namespace Engine {

	class CollisionListener : public b2ContactListener
	{
	public:

		CollisionListener();
		~CollisionListener();

		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;
	};
}
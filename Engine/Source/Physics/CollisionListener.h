#pragma once

#include <box2d/b2_world_callbacks.h>

namespace Engine {
	
	class Level;

	class CollisionListener : public b2ContactListener, public b2ContactFilter
	{
	public:
		CollisionListener(Level* level);
	private:
		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;

		bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB) override;
	private:
		Level* m_Level = nullptr;
	};
}
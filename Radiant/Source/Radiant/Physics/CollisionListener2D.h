#pragma once

#include <box2d/b2_world_callbacks.h>

namespace Radiant {

	class Level;

	class CollisionListener2D : public b2ContactListener, public b2ContactFilter
	{
	public:
		CollisionListener2D(Level* level);
		~CollisionListener2D();
	private:
		virtual void BeginContact(b2Contact* contact) override;
		virtual void EndContact(b2Contact* contact) override;

		virtual bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB) override;
	private:
		Level* m_Level = nullptr;
	};
}
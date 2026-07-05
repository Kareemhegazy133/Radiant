#pragma once

#include <box2d/b2_world_callbacks.h>

namespace Radiant {

	class Level;

	/**
	 * Routes Box2D contact events back to gameplay: resolves both bodies to
	 * entities via the UUIDs stamped in body user data, then invokes the
	 * OnCollisionBegin/OnCollisionEnd callbacks on their RigidBody2DComponents,
	 * passing each the OTHER entity. Owned by Physics2D (created in Init, deleted
	 * in Shutdown); holds a non-owning Level pointer.
	 *
	 * Callbacks fire DURING b2World::Step — Box2D forbids creating/destroying
	 * bodies or otherwise mutating the world from inside them. There are currently
	 * no validity checks on mid-destruction entities; callbacks and filtering are
	 * replaced by queued collision events in the Phase 2 rework (RAD-29).
	 */
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
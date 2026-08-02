#pragma once

#include <cstdint>

namespace Radiant {

	/**
	 * What a teleport does to the body's velocity on arrival. A teleport never
	 * sweeps: it does not collide along the way and nothing between the two
	 * poses is consulted — only what the body is doing once it lands differs.
	 *
	 * KeepVelocity is the default because it is what a teleport physically IS:
	 * the body keeps its momentum through the discontinuity, so a crate falling
	 * at 8 m/s is still falling at 8 m/s on the other side. Portals, wrap-around
	 * level edges, conveyor loops.
	 *
	 * ResetVelocity exists because respawning is not teleporting. A body
	 * teleported mid-landing carries the transient angular velocity of that
	 * landing into open air and tumbles all the way down — correct physics, and
	 * the wrong answer for "put the player back at the checkpoint". It zeroes
	 * linear AND angular velocity; it does NOT cancel forces already applied
	 * this step, because Box2D v3.1 exposes no way to (see PhysicsWorld2D::
	 * Teleport).
	 *
	 * This lives in its own header, apart from PhysicsWorld2D.h, purely so the
	 * ECS layer can name it without pulling in <box2d/id.h> — the same reason
	 * ContactEvent.h exists. UE's equivalent is ETeleportType, whose third mode
	 * (a swept move that collides along the way) has no counterpart here until
	 * the physics query API lands.
	 */
	enum class TeleportType : uint8_t
	{
		// Momentum survives the jump. Value 0, so a zero-initialized
		// TeleportType is the pre-existing behaviour rather than a surprise.
		KeepVelocity,

		// Arrives motionless: linear and angular velocity both zeroed
		ResetVelocity
	};

}
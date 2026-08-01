#pragma once

#include "Radiant/Core/UUID.h"

namespace Radiant {

	/** Which edge of a touch a ContactEvent describes. */
	enum class ContactPhase : uint8_t
	{
		Begin,  // the two shapes started touching
		End     // the two shapes stopped touching
	};

	/**
	 * One "these two entities started/stopped touching" record, translated to
	 * engine types at the module boundary — Box2D types never leave Physics/.
	 * The A/B order is Box2D's shape ordering and carries no meaning: A is not
	 * the instigator.
	 *
	 * An id is 0 when that side could not be resolved because its shape was
	 * already destroyed. That is possible on End only, and it is NORMAL, not a
	 * failure: Box2D v3 reports an end event one step AFTER the destroy that
	 * caused it (see PhysicsWorld2D::Step). Both ids are 0 when both entities
	 * died; consumers skip such records. The surviving side is still told — an
	 * overlap counter that never decrements is worse than one told "you stopped
	 * touching something that is gone".
	 *
	 * Produced by PhysicsWorld2D::Step, consumed by Level within the same fixed
	 * update. This lives in its own header, apart from PhysicsWorld2D.h, purely
	 * so the ECS layer can name ContactPhase without pulling in <box2d/id.h>.
	 */
	struct ContactEvent
	{
		// Entities first, as in BodyMoveEvent — and it keeps the 8-byte ids
		// naturally aligned instead of padding around the enum
		UUID EntityA;
		UUID EntityB;
		ContactPhase Phase;
	};

}
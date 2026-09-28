#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

/**
 * RAD-90 verification scaffolding: a kinematic platform that slides back and
 * forth, driven by MoveKinematic every fixed step (retires with RAD-92).
 *
 * What it proves is the thing that cannot be proved any other way — that a
 * kinematic body moved by TARGET TRANSFORM carries what rests on it. Watch the
 * crate spawned on top: it should ride the platform, not be left behind and
 * not be passed through. Setting the platform's transform per step instead
 * would give it no velocity, so friction would have nothing to act on and the
 * platform would simply slide out from under the crate.
 *
 * It is also the worked example of the every-step contract. Box2D sets the
 * velocity and walks away — a kinematic body that stops receiving targets
 * keeps moving at its last velocity forever — so this script re-issues a
 * target every OnUpdate rather than "starting" a movement once.
 *
 * Bindings are code-only, so GameLayer binds this after spawning the entity.
 */
class KinematicPlatform : public EntityBehaviour
{
public:
	void OnCreate() override
	{
		// The oscillation is measured from wherever the entity was placed, so
		// the script carries no opinion about where the platform lives
		m_Origin = GetOwner().GetComponent<TransformComponent>().Translation;
	}

	void OnUpdate(Timestep ts) override
	{
		// ts is the FIXED delta (OnUpdate runs per simulation step, not per
		// frame), so this phase advances at the same rate at any framerate
		m_Phase += ts * s_AngularSpeed;

		// Absolute target from the phase, never an increment of the current
		// position: reading back the body's own position and adding to it
		// would accumulate the solver's rounding into a drift, and this
		// platform is meant to still be centred on m_Origin an hour from now.
		glm::vec2 target = { m_Origin.x + glm::sin(m_Phase) * s_Amplitude, m_Origin.y };

		// Every step, deliberately. Stop calling this and the platform sails
		// away at its last computed velocity — that is Box2D's contract, not
		// an oversight (see PhysicsBody::MoveKinematic).
		if (PhysicsBody body = GetOwner().GetPhysicsBody())
			body.MoveKinematic(target, 0.0f);
	}

private:
	// Half the travel, in world units, and the radians per second the phase
	// advances — 0.8 rad/s is roughly one full sweep every eight seconds, slow
	// enough to watch the crate's friction do its work
	static constexpr float s_Amplitude = 3.0f;
	static constexpr float s_AngularSpeed = 0.8f;

	glm::vec3 m_Origin{ 0.0f };
	float m_Phase = 0.0f;
};
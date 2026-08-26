#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

/**
 * RAD-29 verification scaffolding: logs every collision this entity takes part
 * in, and optionally destroys the other participant from inside the handler.
 *
 * The destroy mode is the point of this script. Deleting an entity from a
 * collision callback is the exact thing Box2D v2's contact listener made
 * impossible (it mutated the world mid-step) and the exact thing the event
 * queue exists to allow. It also exercises both dispatch guards at once: the
 * partner is re-resolved before every later callback in the same batch, and the
 * surviving side receives its OnCollisionEnd one step later with an INVALID
 * `other` — the destroyed entity's shape is gone by the time Box2D reports the
 * end (v3 buffers end events a step behind).
 *
 * Bindings are code-only, so GameLayer re-binds this after level load.
 */
class CollisionLogger : public EntityBehaviour
{
public:
	/** Destroy whatever this entity first touches, from inside the handler. */
	void SetDestroyOnContact(bool destroyOnContact) { m_DestroyOnContact = destroyOnContact; }

	void OnCollisionBegin(Entity other) override
	{
		// other is invalid only when the partner died before delivery — never
		// on a Begin in practice, but the contract allows it, so honour it
		GAME_TRACE("[collision] BEGIN  {0} <- {1}", Name(), other ? other.Name() : "<destroyed>");

		if (!m_DestroyOnContact || !other)
			return;

		// The world is idle here; this is legal and is the behaviour RAD-29
		// exists to make safe. Any remaining events in this batch that name
		// `other` will resolve it as dead and skip it.
		GAME_WARN("[collision] {0} destroying {1} from inside the handler", Name(), other.Name());

		// Captured BEFORE the destroy, because the name is unreadable after it
		const std::string name = other.Name();
		other.Destroy();

		// THE LINE THIS PROBE EXISTS FOR (retires with RAD-92). Before RAD-97
		// the destroy above had to be the last statement in this method, because
		// destroying an entity freed it outright; this line would have been
		// reading a corpse. It now reports false and nothing is freed until the
		// end of the step, so the method simply carries on.
		//
		// A probe that destroyed as its last statement would pass identically
		// against the old, broken code — the work AFTER the destroy is the whole
		// test, which is why this line must not be "tidied away".
		GAME_TRACE("[collision] post-destroy: '{0}' IsValid={1} (expected false), and this handler is still running",
			name, other.IsValid());

		// GAME_TRACE compiles to ((void)0) in Dist (Log.h), so the line above
		// vanishes in exactly the build where the old use-after-free would
		// actually bite — a probe that silently stops proving anything. So state
		// the invariant as a CHECK instead: silence is the pass, and a violation
		// shouts at a level Dist keeps. Reason from what the log must show under
		// the bug, then confirm it doesn't (playbook §10).
		if (other.IsValid())
			GAME_WARN("[collision] FAIL: '{0}' still reports valid after Destroy() - deferred destruction is broken", name);
	}

	void OnCollisionEnd(Entity other) override
	{
		// An invalid partner here is the NORMAL "the thing I was touching was
		// deleted" case, delivered a step after the deletion
		GAME_TRACE("[collision] END    {0} <- {1}", Name(), other ? other.Name() : "<destroyed>");
	}

private:
	// Name() is on Entity, not EntityBehaviour — reach it through the
	// entity this script is attached to
	const std::string& Name() { return GetOwner().Name(); }

	bool m_DestroyOnContact = false;
};
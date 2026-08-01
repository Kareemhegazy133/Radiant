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
class CollisionLogger : public ScriptableEntity
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
		other.Destroy();
	}

	void OnCollisionEnd(Entity other) override
	{
		// An invalid partner here is the NORMAL "the thing I was touching was
		// deleted" case, delivered a step after the deletion
		GAME_TRACE("[collision] END    {0} <- {1}", Name(), other ? other.Name() : "<destroyed>");
	}

private:
	// Name() is on Entity, not ScriptableEntity — reach it through the
	// entity this script is attached to
	const std::string& Name() { return GetComponent<MetadataComponent>().Tag; }

	bool m_DestroyOnContact = false;
};
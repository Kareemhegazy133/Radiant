#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

/**
 * RAD-95 verification scaffolding: proves a script can spawn and destroy
 * entities from inside the script pass without corrupting it (retires with
 * RAD-92).
 *
 * What it proves is the thing that cannot be proved any other way. Before
 * RAD-95, `Level::OnFixedUpdate` ran scripts inside a live entt view, and no
 * script could reach CreateEntity/DestroyEntity — so the rule "never mutate the
 * iterated pool" held by accident. RAD-95 hands gameplay exactly that power,
 * which is why the pass now snapshots its script set first. This script is the
 * thing that actually walks into it.
 *
 * TWO DETAILS ARE LOAD-BEARING, and a simpler probe would pass against broken
 * code:
 *
 * 1. The spawned entity gets a BOUND SCRIPT. Creating a bare entity never
 *    touches the NativeScriptComponent pool being iterated; binding one does,
 *    and that is the reallocation the snapshot exists to survive.
 * 2. The spawn happens in OnUpdate, NOT in the key handler. GameLayer's cheat
 *    only sets a flag. Spawning straight from OnKeyPressed would run outside
 *    the script pass entirely and prove nothing about it.
 *
 * It also demonstrates the ordering contract in the log rather than asserting
 * it: the passenger's OnCreate appears on the step AFTER the spawn, because the
 * script set was snapshotted before this OnUpdate ran.
 *
 * Watch for, over repeated presses: alternating spawn/destroy traces, one
 * passenger OnCreate per spawn and one OnDestroy per destroy, and no crash in
 * Dist (where the undefined behaviour this guards against would actually bite).
 */
class ProbePassenger : public EntityBehaviour
{
public:
	void OnCreate() override { GAME_TRACE("[spawn-probe] passenger OnCreate"); }

	// Silent: this runs every fixed step, and the point is only that it runs
	void OnUpdate(Timestep ts) override {}

	void OnDestroy() override { GAME_TRACE("[spawn-probe] passenger OnDestroy"); }
};

class SpawnProbe : public EntityBehaviour
{
public:
	/** Called from GameLayer's cheat key; the work happens on the next step. */
	void RequestToggle() { m_ToggleRequested = true; }

	void OnUpdate(Timestep ts) override
	{
		// Toggles on a timer as well as on the cheat key. Deliberate: the
		// behaviour under test needs no human, so a headless run can exercise
		// it — and a headless DIST run is where the undefined behaviour this
		// guards against would actually surface, since the Debug allocator
		// hides pool reallocation.
		m_AutoTimer += ts;
		if (m_AutoTimer >= s_AutoInterval)
		{
			m_AutoTimer = 0.0f;
			m_ToggleRequested = true;
		}

		if (!m_ToggleRequested)
			return;
		m_ToggleRequested = false;

		if (m_HasSpawned)
			DestroySpawned();
		else
			Spawn();
	}

private:
	void Spawn()
	{
		// Level-scope verb, so it comes off GetLevel() rather than the entity —
		// the dividing rule RAD-95 established
		Entity spawned = GetLevel().CreateEntity("ProbeSpawn");
		spawned.AddComponent<SpriteComponent>(glm::vec4{ 0.2f, 0.8f, 1.0f, 1.0f });
		spawned.GetComponent<TransformComponent>().Translation = { -6.0f, 2.0f, 0.0f };

		// THE line under test: this emplaces into the pool currently being
		// iterated by the script pass that is running us
		spawned.AddComponent<NativeScriptComponent>().Bind<ProbePassenger>();

		// Remember the UUID, never the handle: handles are transient and the
		// entity has to survive until the next press (Entity.h's contract)
		m_SpawnedID = spawned.GetUUID();
		m_HasSpawned = true;

		GAME_TRACE("[spawn-probe] spawned '{0}' from inside the script pass - its OnCreate should appear NEXT step", spawned.Name());
	}

	void DestroySpawned()
	{
		// Resolved by UUID, and checked: a level-wide callback or another
		// script could legitimately have destroyed it already
		if (Entity spawned = GetLevel().GetEntityByUUID(m_SpawnedID))
		{
			GAME_TRACE("[spawn-probe] destroying '{0}' from inside the script pass", spawned.Name());
			spawned.Destroy();
		}
		else
		{
			GAME_WARN("[spawn-probe] the spawned entity was already gone");
		}

		m_HasSpawned = false;
	}

	// Seconds of SIMULATED time between automatic toggles — ts is the fixed
	// delta, so this is framerate-independent
	static constexpr float s_AutoInterval = 2.0f;

	UUID m_SpawnedID = UUID(0);
	float m_AutoTimer = 0.0f;
	bool m_HasSpawned = false;
	bool m_ToggleRequested = false;
};
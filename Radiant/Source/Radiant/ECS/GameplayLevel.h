#pragma once

#include "Level.h"

namespace Radiant {

	/**
	 * The Level as gameplay is allowed to see it: spawn, find and destroy
	 * entities, and subscribe to the level-wide collision channel. Obtained
	 * from Entity::GetLevel() or EntityBehaviour::GetLevel(), never
	 * constructed by gameplay.
	 *
	 * WHY THIS TYPE EXISTS (RAD-95). Level is also the frame driver — it owns
	 * OnFixedUpdate, OnRender and OnViewportResize, which belong to whoever
	 * drives the loop (Reaper's GameLayer) and to nothing else. A script
	 * calling OnFixedUpdate would step the physics world from inside the
	 * physics step. Handing gameplay a raw Level* would make that a
	 * one-keystroke mistake, so gameplay gets this instead: the safe subset,
	 * written down where the compiler can read it. Unreal narrows the same
	 * surface by tagging Blueprint-visible functions with UFUNCTION; without a
	 * reflection system (RAD-72) our version of "the tagged subset" has to be
	 * a type.
	 *
	 * Ownership: owns nothing. It holds one non-owning Level* and forwards.
	 * Deliberately NOT a Ref<Level> — the GameLayer owns the level, and a
	 * gameplay handle that could keep it alive would invert that silently.
	 *
	 * Lifetime & threading: TRANSIENT, exactly like Entity and PhysicsBody.
	 * Obtain it, use it, drop it; never store one across a level transition.
	 * Note the asymmetry with Entity: an Entity can detect its own entity
	 * dying (the entt handle carries a generation), but nothing here can
	 * detect the LEVEL dying — a raw Level* has no generation to check. Main-
	 * thread only. 8 bytes; copy freely.
	 *
	 * WHAT IS DELIBERATELY ABSENT, so the next addition meets a decision
	 * rather than a precedent:
	 *   - OnFixedUpdate / OnRender / OnViewportResize — the reason this type
	 *     exists (above).
	 *   - CreateEntityWithUUID — the deserialization path. A caller-chosen
	 *     UUID that collides silently replaces the previous entity's entry in
	 *     the level's lookup map; gameplay has no reason to hold that.
	 *   - GetAssetList — a tooling and serialization concern.
	 *   - SetName — an authoring operation. Running the game never mutates
	 *     authored content.
	 *   - Teleport / RefreshCollider — entity-scope verbs. They live on
	 *     Entity, per the dividing rule below.
	 *   - Any accessor returning the underlying Level*, or a conversion to
	 *     one. That would hand back exactly what this type exists to withhold.
	 *     NEVER ADD ONE.
	 *
	 * THE DIVIDING RULE: a verb that names one entity lives on Entity; a verb
	 * about the level as a whole lives here. Entity::Teleport, Entity::Destroy
	 * and Entity::RefreshCollider follow it from the entity side.
	 */
	class GameplayLevel
	{
	public:
		/** An invalid handle: every verb on it is a warned no-op. */
		GameplayLevel() = default;

		/** Use Entity::GetLevel() — explicit so a Level* never converts silently. */
		explicit GameplayLevel(Level* level)
			: m_Level(level) {}

		/** True when this handle names a live level. */
		bool IsValid() const { return m_Level != nullptr; }
		operator bool() const { return IsValid(); }

		// --- Entities -------------------------------------------------------

		/**
		 * Creates a live entity immediately, with a fresh UUID, a transform and
		 * a metadata tag. Called from a script, the new entity is fully usable
		 * at once, but does not run its OWN script until the next fixed step
		 * (see Level::OnFixedUpdate's script-pass contract). Returns an invalid
		 * Entity if this handle has no level.
		 */
		Entity CreateEntity(const std::string& name = std::string());

		/**
		 * Destroys the entity immediately — see Level::DestroyEntity for the
		 * full contract. Safe from a script or a collision handler; destroying
		 * the entity whose script is currently running deletes that instance,
		 * so it must be the last statement in the handler (a wart RAD-97
		 * removes).
		 */
		void DestroyEntity(Entity entity);

		/** UUID overload of DestroyEntity; a no-op for unknown UUIDs. */
		void DestroyEntity(UUID entityID);

		/**
		 * First entity whose tag matches, or an invalid Entity. O(n) over every
		 * entity in the level — resolve once in OnCreate and remember the UUID
		 * (never the handle, which dangles); do NOT call this per frame.
		 */
		Entity FindEntityByName(std::string_view name);

		/**
		 * Resolves a persistent UUID to a live entity, or an invalid Entity.
		 * UUIDs are the durable identity — Entity handles are transient and are
		 * never serialized, so this is how gameplay remembers a thing.
		 */
		Entity GetEntityByUUID(UUID uuid);

		// --- The level-wide collision channel --------------------------------

		/**
		 * Registers a callback invoked once per collision with BOTH
		 * participants — the channel for systems that belong to no single
		 * entity (damage, audio, VFX). These run before per-entity script
		 * hooks. See Level::AddCollisionCallback for the full contract.
		 *
		 * A SCRIPT THAT REGISTERS ONE MUST REMOVE IT IN OnDestroy. The Level
		 * owns the callback by value and therefore owns its captures; a
		 * callback capturing `this` outlives the script instance it belongs to,
		 * and the Level has no way to notice that its subscriber died. The next
		 * collision then calls into freed memory.
		 */
		Level::CollisionCallbackHandle AddCollisionCallback(std::function<void(const Level::CollisionEvent&)> callback);

		/**
		 * Unregisters the callback and releases its captures. Stale, invalid
		 * and already-removed handles are benign no-ops — this is the correct
		 * idiom for "remove if still registered", and is safe to call from
		 * inside a collision callback, including on itself.
		 */
		void RemoveCollisionCallback(Level::CollisionCallbackHandle& handle);

		// --- Identity --------------------------------------------------------

		/** The level's persistent identity, or a null UUID with no level. */
		UUID GetUUID() const;

		/** The level's name, for logging; a placeholder with no level. */
		const std::string& GetName() const;

	private:
		/**
		 * This layer's single guard (playbook §4, one resolver per layer): a
		 * level-less handle warns, naming the verb, and every verb becomes a
		 * no-op. Verbs carry no preamble of their own, so adding one cannot get
		 * the guard subtly wrong. Everything below this — invalid entity
		 * handles, missing components — is already Level's job to report, and
		 * is deliberately not duplicated here.
		 */
		Level* Resolve(const char* verb) const;

		Level* m_Level = nullptr;

		// Returned by GetName() when there is no level to ask
		inline static const std::string NoLevelName = "<no level>";
	};

}
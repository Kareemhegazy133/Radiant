#pragma once

#include <string>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Radiant/Core/UUID.h"
#include "Radiant/Renderer/SceneCamera.h"
#include "Radiant/Renderer/Texture.h"

// ADDING A NEW COMPONENT
// ----------------------
// If you add a new type of component, there are several pieces of code that need updating:
// 1) Add new component here (obviously).
// 2) Update LevelSerializer to (de)serialize the new component.
// 3) If it contains an asset, update GetAssetList() in Level.cpp
// 4) Decide whether it is ENGINE-PRIVATE — engine-written runtime state that game
//    code must never add, replace or remove — and if so specialize
//    IsEngineComponent<T> at the bottom of this file (RAD-97). Skipping this is
//    silent: the component simply stays writable by every consumer.
//
// Components are meant to be plain data: trivially copyable and serializable, no
// owning pointers, no std::function. The one remaining violation
// (NativeScriptComponent, tracked by RAD-30) is marked — do not add new ones.

namespace Radiant {

	/**
	 * Identity every entity carries (added automatically at creation): persistent
	 * UUID, human-readable tag, and active flag. IsActive gates script updates,
	 * the physics move-event writeback, and rendering — the body itself keeps
	 * simulating (see Docs/Physics.md) — and is not serialized yet (known
	 * round-trip gap).
	 */
	struct MetadataComponent
	{
		UUID ID;
		std::string Tag;
		bool IsActive = true;

		MetadataComponent() = default;
		MetadataComponent(const MetadataComponent&) = default;
		MetadataComponent(const std::string& tag, bool isActive = true)
			: Tag(tag), IsActive(isActive) {}

	};

	/**
	 * World-space placement (added automatically at creation). Rotation is Euler
	 * angles in RADIANS; GetTransform composes translate * rotate * scale on
	 * demand. For entities with a dynamic rigidbody, the physics readback
	 * overwrites Translation.xy and Rotation.z — physics owns those values.
	 */
	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	/**
	 * RUNTIME-ONLY (never serialized — deliberately absent from LevelSerializer):
	 * the entity's transform as of the START of the last fixed simulation step,
	 * written by Level::OnFixedUpdate for entities that can move in simulation.
	 * Level::OnRender draws lerp(snapshot, current, alpha) so fixed-rate motion
	 * looks smooth at any display rate. No scale — nothing simulates scale.
	 * Trivially copyable POD, so Level::Copy (Phase 5) shallow-copies it safely.
	 * Level::Teleport stamps it to the destination pose, so a teleport draws
	 * there immediately instead of smearing across one frame (RAD-28).
	 */
	struct TransformSnapshotComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };

		TransformSnapshotComponent() = default;
		TransformSnapshotComponent(const TransformSnapshotComponent&) = default;
		TransformSnapshotComponent(const glm::vec3& translation, const glm::vec3& rotation)
			: Translation(translation), Rotation(rotation) {}
	};

	/**
	 * RUNTIME-ONLY (never serialized — deliberately absent from LevelSerializer,
	 * like TransformSnapshotComponent above): the condemned notice. Its presence
	 * means Level::DestroyEntity was called on this entity and the entt row has
	 * not been freed yet (RAD-97).
	 *
	 * The window is at most ONE fixed step: the mark adds this tag, and
	 * Level::ReapDestroyedEntities frees the row at the end of that step. For
	 * the whole window Entity::IsValid() reports false and every gameplay-facing
	 * view excludes the entity, so nothing observes it alive — what survives is
	 * the row in memory, reachable only through a handle someone already held.
	 *
	 * THIS POOL IS THE REAP'S WORKLIST, which is why the marker is a tag rather
	 * than a bit on MetadataComponent. entt packs storage in insertion order, so
	 * iterating it yields the pending entities in mark order, and a step where
	 * nothing died costs the reap a size() check on an empty pool. Collapsing
	 * this into a bool would need a parallel vector beside it — a second source
	 * of truth that can disagree — and could not participate in
	 * entt::exclude<>, which is how Level::GetLiveEntitiesWith keeps every
	 * gameplay pass off corpses.
	 *
	 * EMPTY ON PURPOSE, AND NOT `final`. entt allocates no value array for an
	 * empty, non-final type (page_size = !is_empty_v<T> * ENTT_PACKED_PAGE), so
	 * the tag costs no per-entity storage. A member added "just for debugging",
	 * or a `final` that looks harmless, silently forfeits that.
	 *
	 * Ownership: the Level's registry owns it, like every component — no
	 * allocation, no destructor. Engine-private (see IsEngineComponent below):
	 * game code cannot add or remove it, because doing so would destroy an
	 * entity without the mark's bookkeeping. Level::Copy (RAD-52) must SKIP
	 * entities carrying it rather than copy a corpse into the new level.
	 */
	struct PendingDestroyComponent {};

	/**
	 * Renders the entity as a 2D quad. TextureHandle 0 means flat color;
	 * otherwise the texture is resolved through the AssetManager each frame and
	 * tinted by Color. Assets are referenced by handle, never by path.
	 */
	struct SpriteComponent
	{
		glm::vec4 Color{ 1.0f };
		AssetHandle TextureHandle = 0;
		float TilingFactor = 1.0f;

		SpriteComponent() = default;
		SpriteComponent(const SpriteComponent&) = default;
		SpriteComponent(const AssetHandle& textureHandle)
			: TextureHandle(textureHandle) {}
		SpriteComponent(const glm::vec4& color)
			: Color(color) {}
	};

	/**
	 * Makes the entity a viewpoint. Level::OnRender uses the first camera it finds
	 * with Primary set; nothing renders when no Primary camera exists.
	 * FixedAspectRatio opts the camera out of Level::OnViewportResize.
	 */
	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = false; // TODO: think about moving to Level maybe
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	// Forward declaration
	class EntityBehaviour;

	/**
	 * Binds a native C++ behaviour (an EntityBehaviour subclass) to the entity.
	 * Bind<T>() stores factory/destroy callables; the Level instantiates the
	 * behaviour lazily on the first update after binding (see EntityBehaviour
	 * for the lifecycle contract). One behaviour per entity — RAD-101 makes it
	 * several. Bindings are code-only — never serialized — so they must be
	 * re-bound after level load.
	 *
	 * THIS COMPONENT IS PLUMBING; THE QUERY IS THE API (RAD-100). Gameplay
	 * recovers a concrete behaviour with Entity::GetBehaviour<T>(), which is
	 * checked and returns nullptr on a mismatch. Reaching Instance directly
	 * means writing a static_cast the compiler does not verify: get the type
	 * wrong and it reinterprets a live object rather than failing, corrupting
	 * whatever sits at the assumed member offsets.
	 */
	struct NativeScriptComponent
	{
		// Owning raw pointer — known plain-data violation; moves to a Level-owned side table (RAD-30).
		// Read it through Entity::GetBehaviour<T>(), never by casting it yourself (see above).
		EntityBehaviour* Instance = nullptr;

		std::function<EntityBehaviour* ()> InstantiateScript;
		std::function<void(NativeScriptComponent*)> DestroyScript;

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<EntityBehaviour*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	// Physics

	/**
	 * Makes the entity a Box2D body — component presence IS the physics binding:
	 * entt signals create the body on add and destroy it on remove. For Dynamic
	 * bodies, physics owns the transform's Translation.xy and Rotation.z.
	 * Requires a BoxCollider2DComponent (added after this component) to collide.
	 * Collision notifications do not exist yet — they arrive with RAD-29's
	 * event drain (Box2D v3 reports contacts via post-step event buffers).
	 */
	struct RigidBody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };

		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Packed b2BodyId (b2StoreBodyId/b2LoadBodyId), 0 = no body. A claim
		// ticket, not ownership: the Level's PhysicsWorld2D owns the body and
		// zeroes this on destroy. Runtime-only — never serialized; Level::Copy
		// must zero it and recreate bodies (RAD-30/Phase 5). Kept as uint64_t
		// so this header stays free of vendor includes.
		uint64_t RuntimeBodyId = 0;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent&) = default;
		RigidBody2DComponent(const BodyType& type)
			: Type(type) {}
	};

	/**
	 * Box shape for the entity's rigidbody — component presence IS the physics
	 * binding in both directions: adding this component creates the Box2D shape
	 * (requires an existing RigidBody2DComponent — order matters, asserts
	 * otherwise), removing it destroys the shape. Size is HALF-extents in world
	 * units, multiplied by the transform's scale (defaults produce a 1x1 box
	 * matching a unit sprite); Offset is from the body origin, with no local
	 * rotation — the body carries the world rotation.
	 */
	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		// TODO: move into physics material in the future maybe (Box2D v3's
		// b2SurfaceMaterial maps naturally to a physics-material asset)
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		// No RestitutionThreshold: v3 moved it to the world (b2WorldDef) — a
		// stale key in old .rdlvl files is ignored on load (RAD-27)

		// Whether this shape's touches are reported to gameplay (RAD-29).
		// Defaults ON: Box2D reports TRANSITIONS, not states — a settled stack
		// costs nothing per step — so defaulting off would buy no measurable
		// performance while recreating UE's classic "why isn't my hit event
		// firing" trap (bNotifyRigidBodyCollision defaults false there). Turn
		// it off for shapes nobody listens to, e.g. debris, to skip the
		// dispatch cost. Box2D ORs the flag: a contact reports if EITHER shape
		// has it. Read once when the CONTACT is created, so this is effectively
		// a spawn-time property — see Level::RefreshCollider for what toggling
		// it mid-touch does.
		bool EnableContactEvents = true;

		// Packed b2ShapeId (b2StoreShapeId/b2LoadShapeId), 0 = no shape. Same
		// claim-ticket pattern as RigidBody2DComponent::RuntimeBodyId: the
		// Level's PhysicsWorld2D owns the shape and zeroes this on destroy —
		// including when the body dies and takes its shapes with it.
		// Runtime-only — never serialized; deserialization builds the component
		// fresh, so the field stays 0 until the construct signal stores a ticket.
		uint64_t RuntimeShapeId = 0;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	// Currently not in use (using the ImGui font system instead) — parked until
	// the MSDF text revival (RAD-47)
	struct TextComponent
	{
		std::string TextString;

		AssetHandle FontHandle = 0;
		glm::vec4 Color{ 1.0f };
		float TextSize = 12.0f;
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;

		TextComponent() = default;
		TextComponent(const TextComponent&) = default;
		TextComponent(const std::string& text, float size = 12.0f)
			: TextString(text), TextSize(size) {}
	};

	/**
	 * Marks a component as ENGINE-PRIVATE: engine-written runtime state that
	 * game code must never add, replace or remove. Entity's mutating accessors
	 * static_assert on it (see EntityTemplates.h), so a violation is a compile
	 * error naming the verb to use instead — not a runtime surprise (RAD-97).
	 *
	 * WHY A TRAIT RATHER THAN VISIBILITY. Radiant.h exports this header, and it
	 * must: games need SpriteComponent, TransformComponent, NativeScriptComponent.
	 * Hiding a type is not available either — Level.h is public and names
	 * PendingDestroyComponent inside entt::exclude<> for GetLiveEntitiesWith, so
	 * the type is reachable transitively however the files are arranged. With
	 * Entity::AddComponent<T> a universal setter, a trait is the only place the
	 * compiler can read the decision. UE reaches the same guarantee one level
	 * down: AActor::bActorIsBeingDestroyed is private, readable through
	 * IsPendingKillPending(), and writable only via FMarkActorIsBeingDestroyed —
	 * a struct whose constructor is private, friended to UWorld alone.
	 *
	 * The list is hand-maintained, which is its weakness: a new runtime-only
	 * component that forgets its specialization is silently writable. The file
	 * header's step 4 is the reminder; RAD-30's Level-owned side tables are the
	 * real fix, since game code cannot name a side table at all, and RAD-96
	 * decides whether this trait survives that.
	 *
	 * READ ACCESS IS DELIBERATELY UNRESTRICTED. HasComponent/GetComponent/
	 * TryGetComponent are untouched: asking whether an entity is pending
	 * destruction is a fair question, and Entity::IsValid() already answers the
	 * useful form of it. Only MUTATION is gated.
	 */
	template<typename T>
	struct IsEngineComponent : std::false_type {};

	// Written by Level::DestroyEntity, cleared by Level::ReapDestroyedEntities.
	// Adding this by hand destroys an entity WITHOUT the mark's bookkeeping:
	// OnDestroy never runs (so a script's level-wide collision callback is never
	// unregistered, and the next contact calls into a freed instance), the
	// ordered collider-then-rigidbody teardown is skipped, and the UUID map entry
	// is orphaned. Use Entity::Destroy().
	template<> struct IsEngineComponent<PendingDestroyComponent> : std::true_type {};

	// Written and stripped every fixed step by Level::OnFixedUpdate's snapshot
	// pass. A hand-written one is overwritten or removed within a step, so this
	// is about honesty rather than danger: the pass owns this component.
	template<> struct IsEngineComponent<TransformSnapshotComponent> : std::true_type {};

}
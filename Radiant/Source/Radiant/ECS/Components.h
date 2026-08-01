#pragma once

#include <string>

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
	class ScriptableEntity;

	/**
	 * Binds a native C++ script (a ScriptableEntity subclass) to the entity.
	 * Bind<T>() stores factory/destroy callables; the Level instantiates the
	 * script lazily on the first update after binding (see ScriptableEntity for
	 * the lifecycle contract). One script per entity. Bindings are code-only —
	 * never serialized — so they must be re-bound after level load.
	 */
	struct NativeScriptComponent
	{
		// Owning raw pointer — known plain-data violation; moves to a Level-owned side table (RAD-30)
		ScriptableEntity* Instance = nullptr;

		std::function<ScriptableEntity* ()> InstantiateScript;
		std::function<void(NativeScriptComponent*)> DestroyScript;

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
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

}
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
// owning pointers, no std::function. The violations below are marked and tracked
// (RAD-29/RAD-30) — do not add new ones.

namespace Radiant {

	/**
	 * Identity every entity carries (added automatically at creation): persistent
	 * UUID, human-readable tag, and active flag. IsActive gates script updates,
	 * physics transform submission, and rendering — but is not serialized yet
	 * (known round-trip gap).
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
	 * Until RAD-28's explicit-teleport path resets it, a teleport smears across
	 * one rendered frame.
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

	// Forward declaration
	class Entity;

	/**
	 * Makes the entity a Box2D body — component presence IS the physics binding:
	 * entt signals create the body on add and destroy it on remove. For Dynamic
	 * bodies, physics owns the transform's Translation.xy and Rotation.z.
	 * Requires a BoxCollider2DComponent (added after this component) to collide.
	 */
	struct RigidBody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };

		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// b2Body*, owned by the physics world — known plain-data violation; moves to a Level-owned side table (RAD-30)
		void* RuntimeBody = nullptr;
		// Invoked by CollisionListener2D during the physics step with the OTHER
		// entity when contact begins — do not create/destroy bodies or entities
		// from inside it (Box2D forbids world mutation during callbacks).
		// std::function on a component is a known plain-data violation — replaced
		// by queued collision events in the Phase 2 rework (RAD-29, RAD-30).
		std::function<void(Entity&)> OnCollisionBegin = nullptr;

		// Same contract as OnCollisionBegin, invoked when contact ends (RAD-29, RAD-30)
		std::function<void(Entity&)> OnCollisionEnd = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent&) = default;
		RigidBody2DComponent(const BodyType& type)
			: Type(type) {}
	};

	/**
	 * Box fixture for the entity's rigidbody. Size is HALF-extents in world units,
	 * multiplied by the transform's scale (defaults produce a 1x1 box matching a
	 * unit sprite); Offset is from the body origin. Adding this component requires
	 * an existing RigidBody2DComponent — order matters (asserts otherwise).
	 */
	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		// TODO: move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

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
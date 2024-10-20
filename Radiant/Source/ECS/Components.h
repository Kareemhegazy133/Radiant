#pragma once

#include <typeindex>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Core/UUID.h"
#include "LevelCamera.h"
#include "Renderer/Texture.h"

namespace Radiant {

	struct MetadataComponent
	{
		UUID ID;
		std::string Tag;
		bool IsActive = true;

		// Initialize with 'void' as a default type
		std::type_index Type = typeid(void);

		MetadataComponent() = default;
		MetadataComponent(const MetadataComponent&) = default;
		MetadataComponent(const std::string& tag, bool isActive = true)
			: Tag(tag), IsActive(isActive) {}

	};

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

	struct SpriteComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		AssetHandle Texture = 0;
		float TilingFactor = 1.0f;

		SpriteComponent() = default;
		SpriteComponent(const SpriteComponent&) = default;
		SpriteComponent(const AssetHandle& textureHandle)
			: Texture(textureHandle) {}
		SpriteComponent(const glm::vec4& color)
			: Color(color) {}
	};

	struct CameraComponent
	{
		LevelCamera Camera;
		bool Primary = false; // TODO: think about moving to Level maybe
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	// Forward declaration
	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		std::function<ScriptableEntity* ()> InstantiateScript;
		std::function<void(NativeScriptComponent*)> DestroyScript;

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}

		template<typename T>
		void Bind(ScriptableEntity* abilityOwner)
		{
			InstantiateScript = [abilityOwner]() { return static_cast<ScriptableEntity*>(new T(abilityOwner)); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	// Physics

	// Forward declaration
	class Entity;

	struct RigidBody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };

		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;
		/**
		 * @brief Function pointer for handling collision events when a collision begins.
		 *
		 * This function pointer is used to define custom collision behavior when a collision
		 * between entities starts. It can be assigned to a member function of a class or a lambda
		 * function to handle collision events. The function should take a reference to an Entity
		 * object representing the other entity involved in the collision.
		 * @endcode
		*/
		std::function<void(Entity&)> OnCollisionBegin = nullptr;

		/**
		 * @brief Function pointer for handling collision events when a collision ends.
		 *
		 * This function pointer is used to define custom collision behavior when a collision
		 * between entities ends. It can be assigned to a member function of a class or a lambda
		 * function to handle collision events. The function should take a reference to an Entity
		 * object representing the other entity involved in the collision.
		 * @endcode
		*/
		std::function<void(Entity&)> OnCollisionEnd = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent&) = default;
		RigidBody2DComponent(const BodyType& type)
			: Type(type) {}
	};

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

	struct TextComponent
	{
		std::string TextString;

		AssetHandle FontHandle;
		glm::vec4 Color{ 1.0f };
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;

		TextComponent() = default;
		TextComponent(const TextComponent& other) = default;
	};

}
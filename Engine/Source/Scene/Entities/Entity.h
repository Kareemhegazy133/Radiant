#pragma once

#include "Scene/GameObject.h"

enum class CharacterAnimationState;
enum class AbilityAnimationState;

namespace Engine {

	class Entity : public GameObject
	{
	public:
		Entity(entt::entity handle, Scene* scene)
			: GameObject(handle, scene) {}
		Entity(const Entity& other) = default;

		virtual void OnUpdate(Timestep ts);

	protected:
		virtual void OnCollisionBegin(Entity& other);
		virtual void OnCollisionEnd(Entity& other);

		template<typename T>
		void SetupAnimation(T state, const std::string& textureIdentifier, int frameCount, int frameWidth, int frameHeight,
			int frameWidthPadding, int frameHeightPadding, float frameDuration, bool enableLooping);

	protected:
		uint16_t m_FrameWidth;
		uint16_t m_FrameHeight;
		uint8_t m_FrameWidthPadding = 0;
		uint8_t m_FrameHeightPadding = 0;
	};
}
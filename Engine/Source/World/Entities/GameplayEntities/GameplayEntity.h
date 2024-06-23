#pragma once

#include "World/Entities/Entity.h"

namespace Engine {

	class GameplayEntity : public Entity
	{
	public:
		GameplayEntity(entt::entity handle, World* world)
			: Entity(handle, world) {}
		GameplayEntity(const GameplayEntity& other) = default;

	protected:
		virtual void OnCollisionBegin(GameplayEntity& other);
		virtual void OnCollisionEnd(GameplayEntity& other);

		void SetupAnimation(const std::string& animationName, int frameCount, int frameWidth, int frameHeight,
			int frameWidthPadding, int frameHeightPadding, float frameDuration, bool enableLooping);

	protected:
		uint8_t m_FrameCount;
		uint16_t m_FrameWidth;
		uint16_t m_FrameHeight;
		uint8_t m_FrameWidthPadding = 0;
		uint8_t m_FrameHeightPadding = 0;
	};
}
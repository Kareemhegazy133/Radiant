#include "Enginepch.h"

#include "GameplayEntity.h"

namespace Engine {

	void GameplayEntity::OnCollisionBegin(GameplayEntity& other)
	{

	}

	void GameplayEntity::OnCollisionEnd(GameplayEntity& other)
	{

	}

	// TODO: Support Multiple Animations
	void GameplayEntity::SetupAnimation(const std::string& animationName, int frameCount, int frameWidth, int frameHeight,
		int frameWidthPadding, int frameHeightPadding, float frameDuration, bool enableLooping)
	{
		std::vector<sf::IntRect> frames;
		for (int i = 0; i < frameCount; i++)
		{
			frames.emplace_back(sf::IntRect(frameWidth * i + frameWidthPadding, frameHeightPadding, frameWidth - frameWidthPadding * 2, frameHeight - frameHeightPadding));
		}

		auto& animationComponent = GetComponent<AnimationComponent>();
		animationComponent.AddAnimation(animationName, frames, frameDuration, enableLooping);

		animationComponent.Animations[animationName].FrameWidthPadding = frameWidthPadding;
		animationComponent.Animations[animationName].FrameHeightPadding = frameHeightPadding;
	}
}
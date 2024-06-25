#include "Enginepch.h"

#include "Entity.h"

namespace Engine {

	void Entity::OnUpdate(Timestep ts)
	{
		if (GetComponent<MetadataComponent>().OnUpdate)
		{
			GetComponent<MetadataComponent>().OnUpdate(ts);
		}
	}

	void Entity::OnCollisionBegin(Entity& other)
	{

	}

	void Entity::OnCollisionEnd(Entity& other)
	{

	}

	// TODO: Support Multiple Animations
	void Entity::SetupAnimation(const std::string& animationName, int frameCount, int frameWidth, int frameHeight,
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
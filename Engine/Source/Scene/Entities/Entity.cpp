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

	template<typename T>
	void Entity::SetupAnimation(T state, const std::string& textureIdentifier, int frameCount, int frameWidth, int frameHeight,
		int frameWidthPadding, int frameHeightPadding, float frameDuration, bool enableLooping)
	{
		std::vector<sf::IntRect> frames;
		for (int i = 0; i < frameCount; i++)
		{
			int left = frameWidth * i + frameWidthPadding;
			int top = frameHeightPadding;
			int width = frameWidth - frameWidthPadding * 2;
			int height = frameHeight - frameHeightPadding * 2;
			frames.emplace_back(sf::IntRect(left, top, width, height));
		}

		auto& animationComponent = GetComponent<AnimationComponent>();
		animationComponent.AddAnimation(state, textureIdentifier, frames, frameWidthPadding, frameHeightPadding, frameDuration, enableLooping);
	}

	// Explicit template instantiation
	template void Entity::SetupAnimation<PlayerState>(PlayerState, const std::string&, int, int, int, int, int, float, bool);
	template void Entity::SetupAnimation<AbilityState>(AbilityState, const std::string&, int, int, int, int, int, float, bool);
}
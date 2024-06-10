#include "Enginepch.h"
#include "Entity.h"

namespace Engine {

	Entity::Entity(entt::entity handle, World* world)
		: m_EntityHandle(handle), m_World(world)
	{

	}
	
	void Entity::OnCollisionBegin(Entity& other)
	{

	}

	void Entity::OnCollisionEnd(Entity& other)
	{

	}

	void Entity::SetupAnimation(const std::string& animationName, int frameCount, int frameWidth, int frameHeight, int frameWidthPadding, int frameHeightPadding, float frameDuration)
	{
		std::vector<sf::IntRect> frames;
		for (int i = 0; i < frameCount; i++)
		{
			frames.emplace_back(sf::IntRect(frameWidth * i + frameWidthPadding, frameHeightPadding, frameWidth - frameWidthPadding * 2, frameHeight - frameHeightPadding));
		}

		auto& animationComponent = GetComponent<AnimationComponent>();
		animationComponent.AddAnimation(animationName, frames, frameDuration);

		animationComponent.Animations[animationName].FrameWidthPadding = frameWidthPadding;
		animationComponent.Animations[animationName].FrameHeightPadding = frameHeightPadding;
	}
}
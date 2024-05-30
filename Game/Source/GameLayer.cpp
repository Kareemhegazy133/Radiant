#include "GameLayer.h"

using namespace Engine;

GameLayer::GameLayer() : Layer("GameLayer")
{

}

void GameLayer::OnAttach()
{
	m_ActiveScene = CreateRef<Scene>(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow()));

	auto player = m_ActiveScene->CreateEntity("Player");
	player.AddComponent<SpriteComponent>("Player");

	// Define frames for animations

	std::vector<sf::IntRect> idleFrames;
	for (int i = 0; i < 18; i++)
	{
		idleFrames.push_back(sf::IntRect(902 * i + 1, 1, 902, 902));
	}

	player.AddComponent<AnimationComponent>();
	auto& playerAnimationComponent = player.GetComponent<AnimationComponent>();
	playerAnimationComponent.AddAnimation("Idle", idleFrames, 0.05f);
	playerAnimationComponent.SetAnimation("Idle");
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Timestep ts)
{
	m_ActiveScene->OnUpdate(ts);
}


void GameLayer::OnEvent(Event& e)
{
	
}

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

	std::vector<sf::IntRect> idleFrames;
	for (int i = 0; i < 18; i++)
	{
		idleFrames.push_back(sf::IntRect(902 * i + 1, 1, 902, 902));
	}

	auto& playerAnimationComponent = player.AddComponent<AnimationComponent>();
	playerAnimationComponent.AddAnimation("Idle", idleFrames, 0.05f);
	playerAnimationComponent.SetAnimation("Idle");

	auto& rb2d = player.AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
	player.AddComponent<BoxCollider2DComponent>();

	auto& transform = player.GetComponent<TransformComponent>();
	transform.setScale(0.5f, 0.5f);
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

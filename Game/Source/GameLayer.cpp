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

	/*std::vector<sf::IntRect> idleFrames;
	for (int i = 0; i < 18; i++)
	{
		idleFrames.push_back(sf::IntRect(902 * i + 1, 1, 902, 902));
	}

	auto& playerAnimationComponent = player.AddComponent<AnimationComponent>();
	playerAnimationComponent.AddAnimation("Idle", idleFrames, 0.05f);
	playerAnimationComponent.SetAnimation("Idle");*/

	auto& player_transform = player.GetComponent<TransformComponent>();


	auto& player_rb2d = player.AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
	player.AddComponent<BoxCollider2DComponent>();
	player_transform.setPosition(300.f, 0.f);

	auto platform = m_ActiveScene->CreateEntity("Platform");
	platform.AddComponent<SpriteComponent>("Platform");
	auto& platform_transform = platform.GetComponent<TransformComponent>();
	platform_transform.setPosition(100.f, 600.f);

	auto& platform_rb2d = platform.AddComponent<Rigidbody2DComponent>();
	platform.AddComponent<BoxCollider2DComponent>();

	
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

#include "GameLayer.h"

using namespace Engine;

GameLayer::GameLayer() : Layer("GameLayer")
{

}

void GameLayer::OnAttach()
{
	m_ActiveScene = CreateRef<Scene>(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow()));

	auto dice = m_ActiveScene->CreateEntity("Dice");
	auto& diceTransform = dice.GetComponent<TransformComponent>();
	diceTransform.setPosition(sf::Vector2f(100.f, 300.f));
	dice.AddComponent<SpriteComponent>("Dice");
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Engine::Timestep ts)
{
	m_ActiveScene->OnUpdate(ts);
}


void GameLayer::OnEvent(Engine::Event& e)
{
	
}

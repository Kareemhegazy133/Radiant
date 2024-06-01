#include "GameLayer.h"

GameLayer::GameLayer() : Layer("GameLayer")
{

}

void GameLayer::OnAttach()
{
	m_Level = CreateRef<Level>(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow()));
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Timestep ts)
{
	m_Level->OnUpdate(ts);
	m_Level->OnRender();
}


void GameLayer::OnEvent(Event& e)
{
	
}

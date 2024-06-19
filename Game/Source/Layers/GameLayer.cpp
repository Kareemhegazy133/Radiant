#include "GameLayer.h"

GameLayer::GameLayer() : Layer("GameLayer")
{
	GameApplication::Get().GetTextureManager().loadTexture("Player", "Assets/Player/Reaper/Reaper_Idle.png");
	GameApplication::Get().GetTextureManager().loadTexture("Fireball", "Assets/Abilities/Fireball.png");
	GameApplication::Get().GetTextureManager().loadTexture("Platform", "Assets/Platform.png");

}

void GameLayer::OnAttach()
{
	m_Level = CreateRef<Level>();
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
	m_Level->OnEvent(e);
}

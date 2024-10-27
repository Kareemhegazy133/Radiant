#include "GamePausedState.h"

#include <imgui/imgui.h>

#include "Layers/GameLayer.h"

void GamePausedState::OnEnter()
{
	GAME_INFO("PauseMenu OnEnter");

}

void GamePausedState::OnExit()
{
	GAME_INFO("PauseMenu OnExit");
}

void GamePausedState::OnUpdate(Timestep ts)
{

}

void GamePausedState::OnRender()
{

}

void GamePausedState::OnEvent(Event& e)
{

}

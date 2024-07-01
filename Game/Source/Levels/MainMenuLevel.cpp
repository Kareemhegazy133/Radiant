#include "MainMenuLevel.h"

#include "Layers/GameLayer.h"

MainMenuLevel::MainMenuLevel()
{
	std::cout << "MainMenuLevel Created!" << std::endl;
}

MainMenuLevel::~MainMenuLevel()
{
	std::cout << "MainMenuLevel Destroyed!" << std::endl;
}

void MainMenuLevel::OnUpdate(Timestep ts)
{
	GAME_INFO("MainMenuLevel Updating");
}

void MainMenuLevel::OnRender()
{

}

void MainMenuLevel::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(MainMenuLevel::OnKeyPressed));
	if (e.Handled) return;
}

bool MainMenuLevel::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == Key::X)
	{
		GameLayer::Get().SetGameState(GameLayer::GameState::Playing);
		return true;
	}
	return false;
}

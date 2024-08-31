#include "MainMenu.h"

#include "GameTheme.h"
#include "Layers/GameLayer.h"

using namespace Engine;

MainMenu::MainMenu(GameLayer* gameLayer)
	: m_GameLayer(gameLayer)
{
	Initialize();
}

void MainMenu::Initialize()
{
	// TODO: Make menus and elements to be responsive to all resolutions

	gameTitleText.SetText("THE REAPER");
	gameTitleText.SetFont(GameTheme::GetFont());
	gameTitleText.SetCharacterSize(63);
	gameTitleText.SetPosition({ 65.f, 100.f });
	gameTitleText.SetColor(GameTheme::GetLightTextColor());

	playGameButton.SetPosition({ 90.f, 300.f });
	playGameButton.SetSize(GameTheme::GetMediumButtonSize());
	Text playGameButtonText = Text("Play Game", GameTheme::GetFont(), GameTheme::GetMediumButtonTextSize(), GameTheme::GetLightTextColor());
	playGameButton.SetText(playGameButtonText);
	playGameButton.SetButtonCallback(std::bind(&MainMenu::OnPlayGameButtonClicked, this));
	playGameButton.SetNormalTexture(GameTheme::GetMediumButtonNormalTexture());

	optionsButton.SetPosition({ 90.f, 425.f });
	optionsButton.SetSize(GameTheme::GetMediumButtonSize());
	Text optionsButtonText = Text("Options", GameTheme::GetFont(), GameTheme::GetMediumButtonTextSize(), GameTheme::GetLightTextColor());
	optionsButton.SetText(optionsButtonText);
	optionsButton.SetButtonCallback(std::bind(&MainMenu::OnOptionsButtonClicked, this));
	optionsButton.SetNormalTexture(GameTheme::GetMediumButtonNormalTexture());

	quitGameButton.SetPosition({ 90.f, 550.f });
	quitGameButton.SetSize(GameTheme::GetMediumButtonSize());
	Text quitGameButtonText = Text("Quit Game", GameTheme::GetFont(), GameTheme::GetMediumButtonTextSize(), GameTheme::GetLightTextColor());
	quitGameButton.SetText(quitGameButtonText);
	quitGameButton.SetButtonCallback(std::bind(&MainMenu::OnQuitGameButtonClicked, this));
	quitGameButton.SetNormalTexture(GameTheme::GetMediumButtonNormalTexture());

}

void MainMenu::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<MouseButtonPressedEvent>(ENGINE_BIND_EVENT_FN(MainMenu::OnMouseButtonPressed));
	if (e.Handled) return;

}

bool MainMenu::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	if (e.GetMouseButton() == Mouse::Left)
	{
		sf::Vector2i mousePos = {
			static_cast<int>(Input::GetMousePosition().first),
			static_cast<int>(Input::GetMousePosition().second)
		};

		for (auto element : m_Elements[typeid(Button)])
		{
			Button* button = static_cast<Button*>(element);
			if (button->IsHovered(mousePos))
			{
				button->OnClick();
				return true;
			}
		}
	}
	return false;
}

void MainMenu::OnPlayGameButtonClicked()
{
	//GAME_INFO("Play Game Button Clicked!");
	m_GameLayer->ChangeState(m_GameLayer->GetGameplayState());
}

void MainMenu::OnOptionsButtonClicked()
{
	//GAME_INFO("Options Button Clicked!");
}

void MainMenu::OnQuitGameButtonClicked()
{
	//GAME_INFO("Quit Game Button Clicked!");
	GameApplication::GetWindow().Shutdown();
}

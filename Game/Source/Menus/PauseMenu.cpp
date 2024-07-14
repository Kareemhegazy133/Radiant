#include "PauseMenu.h"

#include "Layers/GameLayer.h"

#include "GameTheme.h"

using namespace Engine;

PauseMenu::PauseMenu()
{
	Initialize();
}

PauseMenu::~PauseMenu()
{

}

void PauseMenu::Initialize()
{
	pausedTitleText.SetText("PAUSED");
	pausedTitleText.SetFont(GameTheme::GetFont());
	pausedTitleText.SetCharacterSize(67);
	pausedTitleText.SetPosition({ 525.f, 25.f });
	pausedTitleText.SetColor(sf::Color(51, 94, 109));

	sf::Color ButtonTextColor = sf::Color(41, 77, 89);
	unsigned int ButtonTextSize = 55;

	resumeButton.SetPosition({ 470.f, 160.f });
	resumeButton.SetSize(GameTheme::GetLargeButtonSize());
	Text resumeButtonText = Text("Resume", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetButtonTextColor());
	resumeButton.SetText(resumeButtonText);
	resumeButton.SetButtonCallback(std::bind(&PauseMenu::OnResumeButtonClicked, this));
	resumeButton.SetTexture(GameTheme::GetLargeButtonNormalTexture());

	optionsButton.SetPosition({ 470.f, 285.f });
	optionsButton.SetSize(GameTheme::GetLargeButtonSize());
	Text optionsButtonText = Text("Options", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetButtonTextColor());
	optionsButton.SetText(optionsButtonText);
	optionsButton.SetButtonCallback(std::bind(&PauseMenu::OnOptionsButtonClicked, this));
	optionsButton.SetTexture(GameTheme::GetLargeButtonNormalTexture());

	mainMenuButton.SetPosition({ 470.f, 410.f });
	mainMenuButton.SetSize(GameTheme::GetLargeButtonSize());
	Text mainMenuButtonText = Text("Main Menu", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetButtonTextColor());
	mainMenuButton.SetText(mainMenuButtonText);
	mainMenuButton.SetButtonCallback(std::bind(&PauseMenu::OnMainMenuButtonClicked, this));
	mainMenuButton.SetTexture(GameTheme::GetLargeButtonNormalTexture());

	quitGameButton.SetPosition({ 470.f, 535.f });
	quitGameButton.SetSize(GameTheme::GetLargeButtonSize());
	Text quitGameButtonText = Text("Quit Game", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetButtonTextColor());
	quitGameButton.SetText(quitGameButtonText);
	quitGameButton.SetButtonCallback(std::bind(&PauseMenu::OnQuitGameButtonClicked, this));
	quitGameButton.SetTexture(GameTheme::GetLargeButtonNormalTexture());

}

void PauseMenu::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<MouseButtonPressedEvent>(ENGINE_BIND_EVENT_FN(PauseMenu::OnMouseButtonPressed));
	if (e.Handled) return;

}

bool PauseMenu::OnMouseButtonPressed(MouseButtonPressedEvent& e)
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

void PauseMenu::OnResumeButtonClicked()
{
	GAME_INFO("Resume Button Clicked!");
	SetVisibility(false);
	GameLayer::SetGameState(GameLayer::GameState::Playing);
}

void PauseMenu::OnOptionsButtonClicked()
{
	GAME_INFO("Options Button Clicked!");
}

void PauseMenu::OnMainMenuButtonClicked()
{
	GAME_INFO("Main Menu Button Clicked!");
	SetVisibility(false);
	GameLayer::SetGameState(GameLayer::GameState::MainMenu);
}

void PauseMenu::OnQuitGameButtonClicked()
{
	GAME_INFO("Quit Game Button Clicked!");
	GameApplication::GetWindow().Shutdown();
}

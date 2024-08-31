#include "PauseMenu.h"

#include "GameTheme.h"
#include "Layers/GameLayer.h"

using namespace Engine;

PauseMenu::PauseMenu(GameLayer* gameLayer)
	: m_GameLayer(gameLayer)
{
	Initialize();
}

void PauseMenu::Initialize()
{
	pausedTitleText.SetText("PAUSED");
	pausedTitleText.SetFont(GameTheme::GetFont());
	pausedTitleText.SetCharacterSize(67);
	pausedTitleText.SetPosition({ 525.f, 25.f });
	pausedTitleText.SetColor(GameTheme::GetLightTextColor());

	resumeButton.SetPosition({ 470.f, 160.f });
	resumeButton.SetSize(GameTheme::GetLargeButtonSize());
	Text resumeButtonText = Text("Resume", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetLightTextColor());
	resumeButton.SetText(resumeButtonText);
	resumeButton.SetButtonCallback(std::bind(&PauseMenu::OnResumeButtonClicked, this));
	resumeButton.SetNormalTexture(GameTheme::GetLargeButtonNormalTexture());

	optionsButton.SetPosition({ 470.f, 285.f });
	optionsButton.SetSize(GameTheme::GetLargeButtonSize());
	Text optionsButtonText = Text("Options", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetLightTextColor());
	optionsButton.SetText(optionsButtonText);
	optionsButton.SetButtonCallback(std::bind(&PauseMenu::OnOptionsButtonClicked, this));
	optionsButton.SetNormalTexture(GameTheme::GetLargeButtonNormalTexture());

	mainMenuButton.SetPosition({ 470.f, 410.f });
	mainMenuButton.SetSize(GameTheme::GetLargeButtonSize());
	Text mainMenuButtonText = Text("Main Menu", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetLightTextColor());
	mainMenuButton.SetText(mainMenuButtonText);
	mainMenuButton.SetButtonCallback(std::bind(&PauseMenu::OnMainMenuButtonClicked, this));
	mainMenuButton.SetNormalTexture(GameTheme::GetLargeButtonNormalTexture());

	quitGameButton.SetPosition({ 470.f, 535.f });
	quitGameButton.SetSize(GameTheme::GetLargeButtonSize());
	Text quitGameButtonText = Text("Quit Game", GameTheme::GetFont(), GameTheme::GetLargeButtonTextSize(), GameTheme::GetLightTextColor());
	quitGameButton.SetText(quitGameButtonText);
	quitGameButton.SetButtonCallback(std::bind(&PauseMenu::OnQuitGameButtonClicked, this));
	quitGameButton.SetNormalTexture(GameTheme::GetLargeButtonNormalTexture());

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
	//GAME_INFO("Resume Button Clicked!");
	m_GameLayer->PopState();
}

void PauseMenu::OnOptionsButtonClicked()
{
	//GAME_INFO("Options Button Clicked!");
}

void PauseMenu::OnMainMenuButtonClicked()
{
	//GAME_INFO("Main Menu Button Clicked!");
	m_GameLayer->ChangeState(m_GameLayer->GetMainMenuState());
}

void PauseMenu::OnQuitGameButtonClicked()
{
	//GAME_INFO("Quit Game Button Clicked!");
	GameApplication::GetWindow().Shutdown();
}

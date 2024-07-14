#include "PauseMenu.h"

#include "Layers/GameLayer.h"

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
	// TODO: Abstract Colors
	sf::Color ButtonTextColor = sf::Color(41, 77, 89);
	pausedTitleText.SetText("PAUSED");
	pausedTitleText.SetFont(FontManager::GetFont("Euljiro"));
	pausedTitleText.SetCharacterSize(67);
	pausedTitleText.SetPosition({ 525.f, 25.f });
	pausedTitleText.SetColor(sf::Color(51, 94, 109));

	resumeButton.SetPosition({ 470.f, 160.f });
	resumeButton.SetSize({ 340.f, 90.f });
	Text resumeButtonText = Text("Resume", FontManager::GetFont("Euljiro"), 55, ButtonTextColor);
	resumeButton.SetText(resumeButtonText);
	resumeButton.SetButtonCallback(std::bind(&PauseMenu::OnResumeButtonClicked, this));
	resumeButton.SetTexture(&TextureManager::GetTexture("LargeButton"));

	optionsButton.SetPosition({ 470.f, 285.f });
	optionsButton.SetSize({ 340.f, 90.f });
	Text optionsButtonText = Text("Options", FontManager::GetFont("Euljiro"), 55, ButtonTextColor);
	optionsButton.SetText(optionsButtonText);
	optionsButton.SetButtonCallback(std::bind(&PauseMenu::OnOptionsButtonClicked, this));
	optionsButton.SetTexture(&TextureManager::GetTexture("LargeButton"));

	mainMenuButton.SetPosition({ 470.f, 410.f });
	mainMenuButton.SetSize({ 340.f, 90.f });
	Text mainMenuButtonText = Text("Main Menu", FontManager::GetFont("Euljiro"), 55, ButtonTextColor);
	mainMenuButton.SetText(mainMenuButtonText);
	mainMenuButton.SetButtonCallback(std::bind(&PauseMenu::OnMainMenuButtonClicked, this));
	mainMenuButton.SetTexture(&TextureManager::GetTexture("LargeButton"));

	quitGameButton.SetPosition({ 470.f, 535.f });
	quitGameButton.SetSize({ 340.f, 90.f });
	Text quitGameButtonText = Text("Quit Game", FontManager::GetFont("Euljiro"), 55, ButtonTextColor);
	quitGameButton.SetText(quitGameButtonText);
	quitGameButton.SetButtonCallback(std::bind(&PauseMenu::OnQuitGameButtonClicked, this));
	quitGameButton.SetTexture(&TextureManager::GetTexture("LargeButton"));

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

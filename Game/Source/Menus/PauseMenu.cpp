#include "PauseMenu.h"

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
	gamePausedText.SetText("GAME PAUSED");
	gamePausedText.SetFont(FontManager::GetFont("Dimbo_Regular"));
	gamePausedText.SetCharacterSize(50);
	gamePausedText.SetPosition({ 521.f, 185.f });
	gamePausedText.SetColor(sf::Color::White);

	closeButton.SetPosition({ 810.f, 172.f });
	closeButton.SetSize({ 48.f, 51.f });
	closeButton.SetButtonCallback(std::bind(&PauseMenu::OnCloseButtonClicked, this));
	closeButton.SetTexture(&TextureManager::GetTexture("CloseButton"));

	mainMenuButton.SetPosition({ 450.f, 306.f });
	mainMenuButton.SetSize({ 101.f, 108.f });
	mainMenuButton.SetButtonCallback(std::bind(&PauseMenu::OnMainMenuButtonClicked, this));
	mainMenuButton.SetTexture(&TextureManager::GetTexture("MainMenuButton"));

	resumeButton.SetPosition({ 589.f, 306.f });
	resumeButton.SetSize({ 102.f, 108.f });
	resumeButton.SetButtonCallback(std::bind(&PauseMenu::OnResumeButtonClicked, this));
	resumeButton.SetTexture(&TextureManager::GetTexture("ResumeButton"));

	settingsButton.SetPosition({ 729.f, 306.f });
	settingsButton.SetSize({ 101.f, 108.f });
	settingsButton.SetButtonCallback(std::bind(&PauseMenu::OnSettingsButtonClicked, this));
	settingsButton.SetTexture(&TextureManager::GetTexture("SettingsButton"));

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

		std::vector<Button> m_Buttons;
		m_Buttons.emplace_back(closeButton);
		m_Buttons.emplace_back(mainMenuButton);
		m_Buttons.emplace_back(resumeButton);
		m_Buttons.emplace_back(settingsButton);

		for (auto button : m_Buttons)
		{
			if (button.IsHovered(mousePos))
			{
				button.OnClick();
				return true;
			}
		}
	}
	return false;
}

void PauseMenu::OnCloseButtonClicked()
{
	GAME_INFO("Close Button Clicked!");
}

void PauseMenu::OnMainMenuButtonClicked()
{
	GAME_INFO("MainMenu Button Clicked!");
}

void PauseMenu::OnResumeButtonClicked()
{
	GAME_INFO("Resume Button Clicked!");
}

void PauseMenu::OnSettingsButtonClicked()
{
	GAME_INFO("Settings Button Clicked!");
}

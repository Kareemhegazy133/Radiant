#include "MainMenu.h"

#include "Layers/GameLayer.h"

using namespace Engine;

MainMenu::MainMenu()
{
	Initialize();
}

MainMenu::~MainMenu()
{

}

void MainMenu::Initialize()
{
	// TODO: Abstract Colors
	sf::Color TextColor = sf::Color(41, 77, 89);
	gameTitleText.SetText("THE REAPER");
	gameTitleText.SetFont(FontManager::GetFont("Euljiro"));
	gameTitleText.SetCharacterSize(63);
	gameTitleText.SetPosition({ 65.f, 100.f });
	gameTitleText.SetColor(TextColor);

	playGameButton.SetPosition({ 90.f, 300.f });
	playGameButton.SetSize({ 280.f, 90.f });
	Text playGameButtonText = Text("Play Game", FontManager::GetFont("Euljiro"), 42, TextColor);
	playGameButton.SetText(playGameButtonText);
	playGameButton.SetButtonCallback(std::bind(&MainMenu::OnPlayGameButtonClicked, this));
	playGameButton.SetTexture(&TextureManager::GetTexture("MediumButton"));

	optionsButton.SetPosition({ 90.f, 425.f });
	optionsButton.SetSize({ 280.f, 90.f });
	Text optionsButtonText = Text("Options", FontManager::GetFont("Euljiro"), 42, TextColor);
	optionsButton.SetText(optionsButtonText);
	optionsButton.SetButtonCallback(std::bind(&MainMenu::OnOptionsButtonClicked, this));
	optionsButton.SetTexture(&TextureManager::GetTexture("MediumButton"));

	quitGameButton.SetPosition({ 90.f, 550.f });
	quitGameButton.SetSize({ 280.f, 90.f });
	Text quitGameButtonText = Text("Quit Game", FontManager::GetFont("Euljiro"), 42, TextColor);
	quitGameButton.SetText(quitGameButtonText);
	quitGameButton.SetButtonCallback(std::bind(&MainMenu::OnQuitGameButtonClicked, this));
	quitGameButton.SetTexture(&TextureManager::GetTexture("MediumButton"));

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
	GAME_INFO("Play Game Button Clicked!");
	SetVisibility(false);
	GameLayer::SetGameState(GameLayer::GameState::Playing);
}

void MainMenu::OnOptionsButtonClicked()
{
	GAME_INFO("Options Button Clicked!");
}

void MainMenu::OnQuitGameButtonClicked()
{
	GAME_INFO("Quit Game Button Clicked!");
	GameApplication::GetWindow().Shutdown();
}

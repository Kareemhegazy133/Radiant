#include "PauseMenu.h"

using namespace Engine;

PauseMenu::PauseMenu()
	: Menu("PauseMenu")
{
	m_Font.loadFromFile("Assets/Font/Mantinia_Regular.otf");
	Initialize();
}

PauseMenu::~PauseMenu()
{

}

void PauseMenu::Initialize()
{
	m_TitleText.SetText("Game Paused");
	m_TitleText.SetFont(m_Font);
	m_TitleText.SetCharacterSize(50);
	m_TitleText.SetPosition({ 400.f, 200.f });
	m_TitleText.SetColor(sf::Color::White);

	//m_ResumeButton = AddComponent<ButtonComponent>(
	//	sf::Vector2f(400.f, 300.f),
	//	sf::Vector2f(200.f, 50.f)
	//);
	//m_ResumeButton.SetButtonCallback(std::bind(&PauseMenu::OnResumeButtonClicked, this));
	//m_ResumeButton.SetFillColor(sf::Color::Green);
	//m_ResumeButton.SetOutlineColor(sf::Color::White);
	//m_ResumeButton.SetOutlineThickness(2.f);

	//m_QuitButton = AddComponent<ButtonComponent>(
	//	sf::Vector2f(400.f, 400.f),
	//	sf::Vector2f(200.f, 50.f)
	//);
	//m_QuitButton.SetButtonCallback(std::bind(&PauseMenu::OnQuitButtonClicked, this));
	//m_QuitButton.SetFillColor(sf::Color::Red);
	//m_QuitButton.SetOutlineColor(sf::Color::White);
	//m_QuitButton.SetOutlineThickness(2.f);

	metadata.OnUpdate = BIND_MEMBER_FUNCTION(PauseMenu::OnUpdate, this);
	Hide();
}

void PauseMenu::OnUpdate(Timestep ts)
{
	m_RenderWindow->draw(m_TitleText);
	/*m_RenderWindow->draw(m_ResumeButton);
	m_RenderWindow->draw(m_QuitButton);*/
}

void PauseMenu::OnResumeButtonClicked()
{
}

void PauseMenu::OnQuitButtonClicked()
{
}

void PauseMenu::Show()
{
	metadata.IsActive = true;
	GAME_INFO("PauseMenu Visible");
}

void PauseMenu::Hide()
{
	metadata.IsActive = false;
	GAME_INFO("PauseMenu Hidden");
}

bool PauseMenu::IsVisible() const
{
	return metadata.IsActive;
}

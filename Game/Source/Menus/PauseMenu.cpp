#include "PauseMenu.h"

using namespace Engine;

PauseMenu::PauseMenu()
{
	m_Font.loadFromFile("Assets/Font/Mantinia_Regular.otf");
	Initialize();
}

PauseMenu::~PauseMenu()
{

}

void PauseMenu::Initialize()
{
	titleText.SetText("GAME PAUSED");
	titleText.SetFont(m_Font);
	titleText.SetCharacterSize(50);
	titleText.SetPosition({ 400.f, 200.f });
	titleText.SetColor(sf::Color::White);

	resumeButton.SetPosition({ 400.f, 300.f });
	resumeButton.SetSize({ 200.f, 50.f });
	resumeButton.SetButtonCallback(std::bind(&PauseMenu::OnResumeButtonClicked, this));
	resumeButton.SetFillColor(sf::Color::Green);
	resumeButton.SetOutlineColor(sf::Color::White);
	resumeButton.SetOutlineThickness(2.f);

	quitButton.SetPosition({ 400.f, 400.f });
	quitButton.SetSize({ 200.f, 50.f });
	quitButton.SetButtonCallback(std::bind(&PauseMenu::OnQuitButtonClicked, this));
	quitButton.SetFillColor(sf::Color::Red);
	quitButton.SetOutlineColor(sf::Color::White);
	quitButton.SetOutlineThickness(2.f);

}

void PauseMenu::OnResumeButtonClicked()
{
}

void PauseMenu::OnQuitButtonClicked()
{
}


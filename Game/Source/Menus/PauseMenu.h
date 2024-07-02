#pragma once

#include <Engine.h>

using namespace Engine;

class PauseMenu : public UIScreen
{
public:
	PauseMenu();
	virtual ~PauseMenu();

	void Initialize() override;

	void OnResumeButtonClicked();
	void OnQuitButtonClicked();

private:
	Text& titleText = AddElement<Text>();
	Button& resumeButton = AddElement<Button>();
	Button& quitButton = AddElement<Button>();

	// TODO: Manage Game font elsewhere
	sf::Font m_Font;
};


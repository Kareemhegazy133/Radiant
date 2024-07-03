#pragma once

#include <Engine.h>

using namespace Engine;

class PauseMenu : public UIScreen
{
public:
	PauseMenu();
	virtual ~PauseMenu();

	void Initialize() override;

	void OnCloseButtonClicked();
	void OnMainMenuButtonClicked();
	void OnResumeButtonClicked();
	void OnSettingsButtonClicked();

private:
	Sprite& menuBackground = AddElement<Sprite>(TextureManager::GetTexture("PauseMenu"));
	Text& gamePausedText = AddElement<Text>();
	Button& closeButton = AddElement<Button>();
	Button& mainMenuButton = AddElement<Button>();
	Button& resumeButton = AddElement<Button>();
	Button& settingsButton = AddElement<Button>();
	
	
};


#pragma once

#include <Engine.h>

using namespace Engine;

class PauseMenu : public UIScreen
{
public:
	PauseMenu();
	virtual ~PauseMenu();

	void Initialize() override;

	void OnEvent(Event& e) override;

private:
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

	void OnResumeButtonClicked();
	void OnOptionsButtonClicked();
	void OnMainMenuButtonClicked();
	void OnQuitGameButtonClicked();
private:
	Sprite& menuBackground = AddElement<Sprite>(TextureManager::GetTexture("PauseMenu"));
	Text& pausedTitleText = AddElement<Text>();
	Button& resumeButton = AddElement<Button>();
	Button& optionsButton = AddElement<Button>();
	Button& mainMenuButton = AddElement<Button>();
	Button& quitGameButton = AddElement<Button>();
	
};


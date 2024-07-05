#pragma once

#include <Engine.h>

using namespace Engine;

class MainMenu : public UIScreen
{
public:
	MainMenu();
	virtual ~MainMenu();

	void Initialize() override;

	void OnEvent(Event& e) override;

private:
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

	//void OnCloseButtonClicked();
	//void OnMainMenuButtonClicked();
	//void OnResumeButtonClicked();
	//void OnSettingsButtonClicked();
private:
	//Sprite& menuBackground = AddElement<Sprite>(TextureManager::GetTexture("MainMenu"));
	Text& mainMenuText = AddElement<Text>();
	Button& closeButton = AddElement<Button>();


};


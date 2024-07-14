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

	void OnPlayGameButtonClicked();
	void OnOptionsButtonClicked();
	void OnQuitGameButtonClicked();
private:
	Sprite& menuBackground = AddElement<Sprite>(TextureManager::GetTexture("MainMenu"));
	Text& gameTitleText = AddElement<Text>();
	Button& playGameButton = AddElement<Button>();
	Button& optionsButton = AddElement<Button>();
	Button& quitGameButton = AddElement<Button>();

};


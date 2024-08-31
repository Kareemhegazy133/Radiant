#pragma once

#include <Engine.h>

using namespace Engine;

class GameLayer;

class MainMenu : public UIScreen
{
public:
	MainMenu(GameLayer* gameLayer);
	~MainMenu() = default;

	void Initialize() override;

	void OnEvent(Event& e) override;

private:
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

	void OnPlayGameButtonClicked();
	void OnOptionsButtonClicked();
	void OnQuitGameButtonClicked();
private:
	Sprite& menuBackground = AddElement<Sprite>(ResourceManager::GetTexture("MainMenu"));
	Text& gameTitleText = AddElement<Text>();
	Button& playGameButton = AddElement<Button>();
	Button& optionsButton = AddElement<Button>();
	Button& quitGameButton = AddElement<Button>();

	GameLayer* m_GameLayer = nullptr;
};


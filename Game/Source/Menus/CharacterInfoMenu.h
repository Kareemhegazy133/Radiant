#pragma once

#include <Engine.h>

using namespace Engine;

class CharacterInfoMenu : public UIScreen
{
public:
	CharacterInfoMenu();
	virtual ~CharacterInfoMenu();

	void Initialize() override;

	void OnEvent(Event& e) override;

private:
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

	void OnCloseButtonClicked();
private:
	Button& closeButton = AddElement<Button>();
	Sprite& menuBackground = AddElement<Sprite>(TextureManager::GetTexture("CharacterInfoMenu"));
	Text& characterInfoTitleText = AddElement<Text>();

};

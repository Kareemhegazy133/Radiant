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

	Sprite& characterIcon = AddElement<Sprite>(TextureManager::GetTexture("PlayerIcon"));
	Text& characterHealthText = AddElement<Text>();
	Text& characterStaminaText = AddElement<Text>();
	Sprite& coinsIcon = AddElement<Sprite>(TextureManager::GetTexture("CoinIcon"));
	Text& characterCoinsText = AddElement<Text>();
	Sprite& diamondsIcon = AddElement<Sprite>(TextureManager::GetTexture("DiamondIcon"));
	Text& characterDiamondsText = AddElement<Text>();
	Text& characterLevelText = AddElement<Text>();
	Text& characterLivesText = AddElement<Text>();

	Text& healthStatText = AddElement<Text>();
	Text& staminaStatText = AddElement<Text>();
	Text& strengthStatText = AddElement<Text>();
	Text& defenseStatText = AddElement<Text>();
	Text& magicStatText = AddElement<Text>();
	Text& speedStatText = AddElement<Text>();
};

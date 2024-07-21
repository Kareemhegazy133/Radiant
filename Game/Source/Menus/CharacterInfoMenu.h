#pragma once

#include <Engine.h>

using namespace Engine;

class CharacterInfoMenu : public UIScreen
{
public:
	CharacterInfoMenu(AttributesComponent& playerAttributes, CharacterComponent& playerStats);
	virtual ~CharacterInfoMenu();

	void Initialize() override;

	void UpdatePlayerInfo();

	void OnEvent(Event& e) override;

private:

	bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

	void OnHealthLeftArrowButtonClicked();
	void OnHealthRightArrowButtonClicked();
	void OnStaminaLeftArrowButtonClicked();
	void OnStaminaRightArrowButtonClicked();
	void OnStrengthLeftArrowButtonClicked();
	void OnStrengthRightArrowButtonClicked();
	void OnDefenseLeftArrowButtonClicked();
	void OnDefenseRightArrowButtonClicked();
	void OnMagicLeftArrowButtonClicked();
	void OnMagicRightArrowButtonClicked();

	void OnConfirmAttributePointsButtonClicked();

	void OnCloseButtonClicked();

private:
	Button& closeButton = AddElement<Button>();
	Sprite& menuBackground = AddElement<Sprite>(TextureManager::GetTexture("CharacterInfoMenu"));

	Sprite& characterIcon = AddElement<Sprite>(TextureManager::GetTexture("PlayerIcon"));
	Text& characterHealthText = AddElement<Text>();
	Text& characterStaminaText = AddElement<Text>();
	Sprite& coinsIcon = AddElement<Sprite>(TextureManager::GetTexture("CoinIcon"));
	Text& characterCoinsText = AddElement<Text>();
	Sprite& diamondsIcon = AddElement<Sprite>(TextureManager::GetTexture("DiamondIcon"));
	Text& characterDiamondsText = AddElement<Text>();
	Text& characterLevelText = AddElement<Text>();
	Text& characterAttributePointsText = AddElement<Text>();

	Text& attributePointsTitleText = AddElement<Text>();
	Text& healthPointsText = AddElement<Text>();
	Button& healthLeftArrowButton = AddElement<Button>();
	Button& healthRightArrowButton = AddElement<Button>();
	Text& staminaPointsText = AddElement<Text>();
	Button& staminaLeftArrowButton = AddElement<Button>();
	Button& staminaRightArrowButton = AddElement<Button>();
	Text& strengthPointsText = AddElement<Text>();
	Button& strengthLeftArrowButton = AddElement<Button>();
	Button& strengthRightArrowButton = AddElement<Button>();
	Text& defensePointsText = AddElement<Text>();
	Button& defenseLeftArrowButton = AddElement<Button>();
	Button& defenseRightArrowButton = AddElement<Button>();
	Text& magicPointsText = AddElement<Text>();
	Button& magicLeftArrowButton = AddElement<Button>();
	Button& magicRightArrowButton = AddElement<Button>();
	Button& confirmAttributePointsButton = AddElement<Button>();

	Text& baseStatsTitleText = AddElement<Text>();
	Text& healthStatText = AddElement<Text>();
	Text& staminaStatText = AddElement<Text>();
	Text& strengthStatText = AddElement<Text>();
	Text& defenseStatText = AddElement<Text>();
	Text& magicStatText = AddElement<Text>();
	Text& speedStatText = AddElement<Text>();

	AttributesComponent& m_playerAttributes;
	CharacterComponent& m_playerStats;

	int m_AttributePointsToSpend = 0;
	int m_HealthPointsToAdd = 0;
	int m_StaminaPointsToAdd = 0;
	int m_StrengthPointsToAdd = 0;
	int m_DefensePointsToAdd = 0;
	int m_MagicPointsToAdd = 0;
};

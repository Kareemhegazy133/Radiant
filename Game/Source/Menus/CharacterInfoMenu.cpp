#include "CharacterInfoMenu.h"

#include "Layers/GameLayer.h"

#include "GameTheme.h"
#include "Gameplay/Attributes.h"

using namespace Engine;

CharacterInfoMenu::CharacterInfoMenu()
{
	Initialize();
}

CharacterInfoMenu::~CharacterInfoMenu()
{

}

void CharacterInfoMenu::Initialize()
{
	characterIcon.SetPosition({ 190.f, 120.f });

	characterHealthText.SetFont(GameTheme::GetFont());
	characterHealthText.SetCharacterSize(25);
	characterHealthText.SetPosition({ 350.f, 120.f });
	characterHealthText.SetColor(GameTheme::GetLightTextColor());

	characterStaminaText.SetFont(GameTheme::GetFont());
	characterStaminaText.SetCharacterSize(25);
	characterStaminaText.SetPosition({ 350.f, 150.f });
	characterStaminaText.SetColor(GameTheme::GetLightTextColor());

	coinsIcon.SetPosition({ 300.f, 193.f });
	characterCoinsText.SetFont(GameTheme::GetFont());
	characterCoinsText.SetCharacterSize(25);
	characterCoinsText.SetPosition({ 335.f, 193.f });
	characterCoinsText.SetColor(GameTheme::GetLightTextColor());

	diamondsIcon.SetPosition({ 420.f, 193.f });
	characterDiamondsText.SetFont(GameTheme::GetFont());
	characterDiamondsText.SetCharacterSize(25);
	characterDiamondsText.SetPosition({ 455.f, 193.f });
	characterDiamondsText.SetColor(GameTheme::GetLightTextColor());

	characterLevelText.SetFont(GameTheme::GetFont());
	characterLevelText.SetCharacterSize(21);
	characterLevelText.SetPosition({ 240.f, 245.f });
	characterLevelText.SetColor(GameTheme::GetDarkTextColor());

	characterLivesText.SetText("Lives : 4");
	characterLivesText.SetFont(GameTheme::GetFont());
	characterLivesText.SetCharacterSize(21);
	characterLivesText.SetPosition({ 400.f, 245.f });
	characterLivesText.SetColor(GameTheme::GetDarkTextColor());

	attributePointsTitleText.SetText("Attribute\n   Points");
	attributePointsTitleText.SetFont(GameTheme::GetFont());
	attributePointsTitleText.SetCharacterSize(30);
	attributePointsTitleText.SetPosition({ 308.f, 335.f });
	attributePointsTitleText.SetColor(GameTheme::GetLightTextColor());

	healthPointsText.SetFont(GameTheme::GetFont());
	healthPointsText.SetCharacterSize(21);
	healthPointsText.SetPosition({ 240.f, 427.f });
	healthPointsText.SetColor(GameTheme::GetLightTextColor());

	healthLeftArrowButton.SetPosition({ 420.f, 430.f });
	healthLeftArrowButton.SetSize({ 27.f, 27.f });
	healthLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnHealthLeftArrowButtonClicked, this));
	healthLeftArrowButton.SetTexture(&TextureManager::GetTexture("LeftArrowButton"));
	healthLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));

	healthRightArrowButton.SetPosition({ 485.f, 430.f });
	healthRightArrowButton.SetSize({ 27.f, 27.f });
	healthRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnHealthRightArrowButtonClicked, this));
	healthRightArrowButton.SetTexture(&TextureManager::GetTexture("RightArrowButton"));
	healthRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));

	staminaPointsText.SetFont(GameTheme::GetFont());
	staminaPointsText.SetCharacterSize(21);
	staminaPointsText.SetPosition({ 240.f, 477.f });
	staminaPointsText.SetColor(GameTheme::GetLightTextColor());

	staminaLeftArrowButton.SetPosition({ 420.f, 480.f });
	staminaLeftArrowButton.SetSize({ 27.f, 27.f });
	staminaLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStaminaLeftArrowButtonClicked, this));
	staminaLeftArrowButton.SetTexture(&TextureManager::GetTexture("LeftArrowButton"));
	staminaLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));

	staminaRightArrowButton.SetPosition({ 485.f, 480.f });
	staminaRightArrowButton.SetSize({ 27.f, 27.f });
	staminaRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStaminaRightArrowButtonClicked, this));
	staminaRightArrowButton.SetTexture(&TextureManager::GetTexture("RightArrowButton"));
	staminaRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));

	strengthPointsText.SetFont(GameTheme::GetFont());
	strengthPointsText.SetCharacterSize(21);
	strengthPointsText.SetPosition({ 240.f, 527.f });
	strengthPointsText.SetColor(GameTheme::GetLightTextColor());

	strengthLeftArrowButton.SetPosition({ 420.f, 530.f });
	strengthLeftArrowButton.SetSize({ 27.f, 27.f });
	strengthLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStrengthLeftArrowButtonClicked, this));
	strengthLeftArrowButton.SetTexture(&TextureManager::GetTexture("LeftArrowButton"));
	strengthLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));

	strengthRightArrowButton.SetPosition({ 485.f, 530.f });
	strengthRightArrowButton.SetSize({ 27.f, 27.f });
	strengthRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStrengthRightArrowButtonClicked, this));
	strengthRightArrowButton.SetTexture(&TextureManager::GetTexture("RightArrowButton"));
	strengthRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));

	defensePointsText.SetFont(GameTheme::GetFont());
	defensePointsText.SetCharacterSize(21);
	defensePointsText.SetPosition({ 240.f, 580.f });
	defensePointsText.SetColor(GameTheme::GetLightTextColor());

	defenseLeftArrowButton.SetPosition({ 420.f, 582.f });
	defenseLeftArrowButton.SetSize({ 27.f, 27.f });
	defenseLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnDefenseLeftArrowButtonClicked, this));
	defenseLeftArrowButton.SetTexture(&TextureManager::GetTexture("LeftArrowButton"));
	defenseLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));

	defenseRightArrowButton.SetPosition({ 485.f, 582.f });
	defenseRightArrowButton.SetSize({ 27.f, 27.f });
	defenseRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnDefenseRightArrowButtonClicked, this));
	defenseRightArrowButton.SetTexture(&TextureManager::GetTexture("RightArrowButton"));
	defenseRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));

	magicPointsText.SetFont(GameTheme::GetFont());
	magicPointsText.SetCharacterSize(21);
	magicPointsText.SetPosition({ 240.f, 630.f });
	magicPointsText.SetColor(GameTheme::GetLightTextColor());

	magicLeftArrowButton.SetPosition({ 420.f, 632.f });
	magicLeftArrowButton.SetSize({ 27.f, 27.f });
	magicLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnMagicLeftArrowButtonClicked, this));
	magicLeftArrowButton.SetTexture(&TextureManager::GetTexture("LeftArrowButton"));
	magicLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));

	magicRightArrowButton.SetPosition({ 485.f, 632.f });
	magicRightArrowButton.SetSize({ 27.f, 27.f });
	magicRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnMagicRightArrowButtonClicked, this));
	magicRightArrowButton.SetTexture(&TextureManager::GetTexture("RightArrowButton"));
	magicRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));

	baseStatsTitleText.SetText("Base Stats");
	baseStatsTitleText.SetFont(GameTheme::GetFont());
	baseStatsTitleText.SetCharacterSize(34);
	baseStatsTitleText.SetPosition({ 770.f, 120.f });
	baseStatsTitleText.SetColor(GameTheme::GetLightTextColor());

	closeButton.SetPosition({ 1035.f, 190.f });
	closeButton.SetSize({ 80.f, 80.f });
	closeButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnCloseButtonClicked, this));
	closeButton.SetTexture(&TextureManager::GetTexture("CloseButton"));
	closeButton.SetHoveredTexture(&TextureManager::GetTexture("CloseButtonHovered"));

	healthStatText.SetFont(GameTheme::GetFont());
	healthStatText.SetCharacterSize(21);
	healthStatText.SetPosition({ 725.f, 205.f });
	healthStatText.SetColor(GameTheme::GetLightTextColor());

	staminaStatText.SetFont(GameTheme::GetFont());
	staminaStatText.SetCharacterSize(21);
	staminaStatText.SetPosition({ 725.f, 254.f });
	staminaStatText.SetColor(GameTheme::GetLightTextColor());

	strengthStatText.SetFont(GameTheme::GetFont());
	strengthStatText.SetCharacterSize(21);
	strengthStatText.SetPosition({ 725.f, 305.f });
	strengthStatText.SetColor(GameTheme::GetLightTextColor());

	defenseStatText.SetFont(GameTheme::GetFont());
	defenseStatText.SetCharacterSize(21);
	defenseStatText.SetPosition({ 725.f, 356.f });
	defenseStatText.SetColor(GameTheme::GetLightTextColor());

	magicStatText.SetFont(GameTheme::GetFont());
	magicStatText.SetCharacterSize(21);
	magicStatText.SetPosition({ 725.f, 406.f });
	magicStatText.SetColor(GameTheme::GetLightTextColor());

	speedStatText.SetFont(GameTheme::GetFont());
	speedStatText.SetCharacterSize(21);
	speedStatText.SetPosition({ 725.f, 457.f });
	speedStatText.SetColor(GameTheme::GetLightTextColor());

}

void CharacterInfoMenu::UpdatePlayerInfo(AttributesComponent& playerAttributes, CharacterComponent& playerStats)
{
	characterHealthText.SetText(
		std::to_string(static_cast<int>(playerStats.CurrentHealth))
		+ "/"
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Health)))
	);

	characterStaminaText.SetText(
		std::to_string(static_cast<int>(playerStats.CurrentStamina))
		+ "/"
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Stamina)))
	);

	characterLevelText.SetText("Level : " + std::to_string(static_cast<int>(playerStats.Level)));
	characterCoinsText.SetText(std::to_string(static_cast<int>(playerStats.Coins)));
	characterDiamondsText.SetText(std::to_string(static_cast<int>(playerStats.Diamonds)));

	healthPointsText.SetText("Health : 1        -->");
	staminaPointsText.SetText("Stamina : 1     -->");
	strengthPointsText.SetText("Strength : 1    -->");
	defensePointsText.SetText("Defense : 1     -->");
	magicPointsText.SetText("Magic : 1         -->");

	healthStatText.SetText("Health : " 
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Health)))
		+ "      -->   "
		+ std::to_string(static_cast<int>(playerAttributes.CalculateUpgradeValue(Attributes::Health)))
	);

	staminaStatText.SetText("Stamina : "
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Stamina)))
		+ "   -->   "
		+ std::to_string(static_cast<int>(playerAttributes.CalculateUpgradeValue(Attributes::Stamina)))
	);

	strengthStatText.SetText("Strength : "
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Strength)))
		+ "    -->   "
		+ std::to_string(static_cast<int>(playerAttributes.CalculateUpgradeValue(Attributes::Strength)))
	);

	defenseStatText.SetText("Defense : "
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Defense)))
		+ "     -->   "
		+ std::to_string(static_cast<int>(playerAttributes.CalculateUpgradeValue(Attributes::Defense)))
	);

	magicStatText.SetText("Magic : "
		+ std::to_string(static_cast<int>(playerAttributes.GetAttribute(Attributes::Magic)))
		+ "         -->   "
		+ std::to_string(static_cast<int>(playerAttributes.CalculateUpgradeValue(Attributes::Magic)))
	);

	speedStatText.SetText("Speed : "
		+ std::to_string(static_cast<int>(playerStats.Speed))
		+ "      -->   "
	);
}

void CharacterInfoMenu::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<MouseButtonPressedEvent>(ENGINE_BIND_EVENT_FN(CharacterInfoMenu::OnMouseButtonPressed));
	if (e.Handled) return;

}

bool CharacterInfoMenu::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
	if (e.GetMouseButton() == Mouse::Left)
	{
		sf::Vector2i mousePos = {
			static_cast<int>(Input::GetMousePosition().first),
			static_cast<int>(Input::GetMousePosition().second)
		};

		for (auto element : m_Elements[typeid(Button)])
		{
			Button* button = static_cast<Button*>(element);
			if (button->IsHovered(mousePos))
			{
				button->OnClick();
				return true;
			}
		}
	}
	return false;
}

void CharacterInfoMenu::OnHealthLeftArrowButtonClicked()
{
	GAME_INFO("Health Left Arrow Clicked");
}

void CharacterInfoMenu::OnHealthRightArrowButtonClicked()
{
	GAME_INFO("Health Right Arrow Clicked");
}

void CharacterInfoMenu::OnStaminaLeftArrowButtonClicked()
{
	GAME_INFO("Stamina Left Arrow Clicked");
}

void CharacterInfoMenu::OnStaminaRightArrowButtonClicked()
{
	GAME_INFO("Stamina Right Arrow Clicked");
}

void CharacterInfoMenu::OnStrengthLeftArrowButtonClicked()
{
	GAME_INFO("Strength Left Arrow Clicked");
}

void CharacterInfoMenu::OnStrengthRightArrowButtonClicked()
{
	GAME_INFO("Strength Right Arrow Clicked");
}

void CharacterInfoMenu::OnDefenseLeftArrowButtonClicked()
{
	GAME_INFO("Defense Left Arrow Clicked");
}

void CharacterInfoMenu::OnDefenseRightArrowButtonClicked()
{
	GAME_INFO("Defense Right Arrow Clicked");
}

void CharacterInfoMenu::OnMagicLeftArrowButtonClicked()
{
	GAME_INFO("Magic Left Arrow Clicked");
}

void CharacterInfoMenu::OnMagicRightArrowButtonClicked()
{
	GAME_INFO("Magic Right Arrow Clicked");
}

void CharacterInfoMenu::OnCloseButtonClicked()
{
	GAME_INFO("Close Button Clicked!");
	SetVisibility(false);
}

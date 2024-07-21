#include "CharacterInfoMenu.h"

#include "Layers/GameLayer.h"

#include "GameTheme.h"
#include "Gameplay/Attributes.h"

using namespace Engine;

CharacterInfoMenu::CharacterInfoMenu(AttributesComponent& playerAttributes, CharacterComponent& playerStats)
	: m_playerAttributes(playerAttributes), m_playerStats(playerStats)
{
	Initialize();
}

CharacterInfoMenu::~CharacterInfoMenu()
{

}

void CharacterInfoMenu::Initialize()
{
	characterIcon.SetPosition({ 190.f, 45.f });

	characterHealthText.SetFont(GameTheme::GetFont());
	characterHealthText.SetCharacterSize(25);
	characterHealthText.SetPosition({ 360.f, 45.f });
	characterHealthText.SetColor(GameTheme::GetLightTextColor());

	characterStaminaText.SetFont(GameTheme::GetFont());
	characterStaminaText.SetCharacterSize(25);
	characterStaminaText.SetPosition({ 360.f, 75.f });
	characterStaminaText.SetColor(GameTheme::GetLightTextColor());

	coinsIcon.SetPosition({ 300.f, 118.f });
	characterCoinsText.SetFont(GameTheme::GetFont());
	characterCoinsText.SetCharacterSize(25);
	characterCoinsText.SetPosition({ 335.f, 118.f });
	characterCoinsText.SetColor(GameTheme::GetLightTextColor());

	diamondsIcon.SetPosition({ 420.f, 118.f });
	characterDiamondsText.SetFont(GameTheme::GetFont());
	characterDiamondsText.SetCharacterSize(25);
	characterDiamondsText.SetPosition({ 455.f, 118.f });
	characterDiamondsText.SetColor(GameTheme::GetLightTextColor());

	characterLevelText.SetFont(GameTheme::GetFont());
	characterLevelText.SetCharacterSize(21);
	characterLevelText.SetPosition({ 240.f, 167.f });
	characterLevelText.SetColor(GameTheme::GetDarkTextColor());

	characterAttributePointsText.SetFont(GameTheme::GetFont());
	characterAttributePointsText.SetCharacterSize(21);
	characterAttributePointsText.SetPosition({ 400.f, 167.f });
	characterAttributePointsText.SetColor(GameTheme::GetDarkTextColor());

	attributePointsTitleText.SetText("Attribute\n   Points");
	attributePointsTitleText.SetFont(GameTheme::GetFont());
	attributePointsTitleText.SetCharacterSize(30);
	attributePointsTitleText.SetPosition({ 290.f, 260.f });
	attributePointsTitleText.SetColor(GameTheme::GetLightTextColor());

	healthPointsText.SetFont(GameTheme::GetFont());
	healthPointsText.SetCharacterSize(21);
	healthPointsText.SetPosition({ 225.f, 353.f });
	healthPointsText.SetColor(GameTheme::GetLightTextColor());

	healthLeftArrowButton.SetPosition({ 400.f, 356.f });
	healthLeftArrowButton.SetSize({ 27.f, 27.f });
	healthLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnHealthLeftArrowButtonClicked, this));
	healthLeftArrowButton.SetNormalTexture(&TextureManager::GetTexture("LeftArrowButton"));
	healthLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));
	healthLeftArrowButton.SetDisabledTexture(&TextureManager::GetTexture("LeftArrowButtonDisabled"));

	healthRightArrowButton.SetPosition({ 465.f, 356.f });
	healthRightArrowButton.SetSize({ 27.f, 27.f });
	healthRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnHealthRightArrowButtonClicked, this));
	healthRightArrowButton.SetNormalTexture(&TextureManager::GetTexture("RightArrowButton"));
	healthRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));
	healthRightArrowButton.SetDisabledTexture(&TextureManager::GetTexture("RightArrowButtonDisabled"));

	staminaPointsText.SetFont(GameTheme::GetFont());
	staminaPointsText.SetCharacterSize(21);
	staminaPointsText.SetPosition({ 225.f, 403.f });
	staminaPointsText.SetColor(GameTheme::GetLightTextColor());

	staminaLeftArrowButton.SetPosition({ 400.f, 406.f });
	staminaLeftArrowButton.SetSize({ 27.f, 27.f });
	staminaLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStaminaLeftArrowButtonClicked, this));
	staminaLeftArrowButton.SetNormalTexture(&TextureManager::GetTexture("LeftArrowButton"));
	staminaLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));
	staminaLeftArrowButton.SetDisabledTexture(&TextureManager::GetTexture("LeftArrowButtonDisabled"));

	staminaRightArrowButton.SetPosition({ 465.f, 406.f });
	staminaRightArrowButton.SetSize({ 27.f, 27.f });
	staminaRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStaminaRightArrowButtonClicked, this));
	staminaRightArrowButton.SetNormalTexture(&TextureManager::GetTexture("RightArrowButton"));
	staminaRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));
	staminaRightArrowButton.SetDisabledTexture(&TextureManager::GetTexture("RightArrowButtonDisabled"));

	strengthPointsText.SetFont(GameTheme::GetFont());
	strengthPointsText.SetCharacterSize(21);
	strengthPointsText.SetPosition({ 225.f, 455.f });
	strengthPointsText.SetColor(GameTheme::GetLightTextColor());

	strengthLeftArrowButton.SetPosition({ 400.f, 456.f });
	strengthLeftArrowButton.SetSize({ 27.f, 27.f });
	strengthLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStrengthLeftArrowButtonClicked, this));
	strengthLeftArrowButton.SetNormalTexture(&TextureManager::GetTexture("LeftArrowButton"));
	strengthLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));
	strengthLeftArrowButton.SetDisabledTexture(&TextureManager::GetTexture("LeftArrowButtonDisabled"));

	strengthRightArrowButton.SetPosition({ 465.f, 456.f });
	strengthRightArrowButton.SetSize({ 27.f, 27.f });
	strengthRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnStrengthRightArrowButtonClicked, this));
	strengthRightArrowButton.SetNormalTexture(&TextureManager::GetTexture("RightArrowButton"));
	strengthRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));
	strengthRightArrowButton.SetDisabledTexture(&TextureManager::GetTexture("RightArrowButtonDisabled"));

	defensePointsText.SetFont(GameTheme::GetFont());
	defensePointsText.SetCharacterSize(21);
	defensePointsText.SetPosition({ 225.f, 506.f });
	defensePointsText.SetColor(GameTheme::GetLightTextColor());

	defenseLeftArrowButton.SetPosition({ 400.f, 508.f });
	defenseLeftArrowButton.SetSize({ 27.f, 27.f });
	defenseLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnDefenseLeftArrowButtonClicked, this));
	defenseLeftArrowButton.SetNormalTexture(&TextureManager::GetTexture("LeftArrowButton"));
	defenseLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));
	defenseLeftArrowButton.SetDisabledTexture(&TextureManager::GetTexture("LeftArrowButtonDisabled"));

	defenseRightArrowButton.SetPosition({ 465.f, 508.f });
	defenseRightArrowButton.SetSize({ 27.f, 27.f });
	defenseRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnDefenseRightArrowButtonClicked, this));
	defenseRightArrowButton.SetNormalTexture(&TextureManager::GetTexture("RightArrowButton"));
	defenseRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));
	defenseRightArrowButton.SetDisabledTexture(&TextureManager::GetTexture("RightArrowButtonDisabled"));

	magicPointsText.SetFont(GameTheme::GetFont());
	magicPointsText.SetCharacterSize(21);
	magicPointsText.SetPosition({ 225.f, 556.f });
	magicPointsText.SetColor(GameTheme::GetLightTextColor());

	magicLeftArrowButton.SetPosition({ 400.f, 558.f });
	magicLeftArrowButton.SetSize({ 27.f, 27.f });
	magicLeftArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnMagicLeftArrowButtonClicked, this));
	magicLeftArrowButton.SetNormalTexture(&TextureManager::GetTexture("LeftArrowButton"));
	magicLeftArrowButton.SetHoveredTexture(&TextureManager::GetTexture("LeftArrowButtonHovered"));
	magicLeftArrowButton.SetDisabledTexture(&TextureManager::GetTexture("LeftArrowButtonDisabled"));

	magicRightArrowButton.SetPosition({ 465.f, 558.f });
	magicRightArrowButton.SetSize({ 27.f, 27.f });
	magicRightArrowButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnMagicRightArrowButtonClicked, this));
	magicRightArrowButton.SetNormalTexture(&TextureManager::GetTexture("RightArrowButton"));
	magicRightArrowButton.SetHoveredTexture(&TextureManager::GetTexture("RightArrowButtonHovered"));
	magicRightArrowButton.SetDisabledTexture(&TextureManager::GetTexture("RightArrowButtonDisabled"));

	confirmAttributePointsButton.SetPosition({ 285.f, 615.f });
	confirmAttributePointsButton.SetSize(GameTheme::GetCommonButtonSize());
	Text confirmAttributePointsButtonText = Text("Confirm", GameTheme::GetFont(), GameTheme::GetCommonButtonTextSize(), GameTheme::GetDisabledTextColor());
	confirmAttributePointsButton.SetText(confirmAttributePointsButtonText);
	confirmAttributePointsButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnConfirmAttributePointsButtonClicked, this));
	confirmAttributePointsButton.SetNormalTexture(GameTheme::GetCommonButtonNormalTexture());
	confirmAttributePointsButton.SetHoveredTexture(GameTheme::GetCommonButtonHoveredTexture());
	confirmAttributePointsButton.SetDisabledTexture(GameTheme::GetCommonButtonDisabledTexture());

	baseStatsTitleText.SetText("Base Stats");
	baseStatsTitleText.SetFont(GameTheme::GetFont());
	baseStatsTitleText.SetCharacterSize(34);
	baseStatsTitleText.SetPosition({ 770.f, 120.f });
	baseStatsTitleText.SetColor(GameTheme::GetLightTextColor());

	closeButton.SetPosition({ 1035.f, 190.f });
	closeButton.SetSize({ 80.f, 80.f });
	closeButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnCloseButtonClicked, this));
	closeButton.SetNormalTexture(&TextureManager::GetTexture("CloseButton"));
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

void CharacterInfoMenu::UpdatePlayerInfo()
{
	characterHealthText.SetText(
		std::to_string(static_cast<int>(m_playerStats.CurrentHealth))
		+ "/"
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Health)))
	);

	characterStaminaText.SetText(
		std::to_string(static_cast<int>(m_playerStats.CurrentStamina))
		+ "/"
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Stamina)))
	);

	characterLevelText.SetText("Level : " + std::to_string(m_playerStats.Level));
	characterAttributePointsText.SetText("Points : " + std::to_string(m_playerAttributes.GetAttributePointsAvailable() - m_AttributePointsToSpend));
	characterCoinsText.SetText(std::to_string(m_playerStats.Coins));
	characterDiamondsText.SetText(std::to_string(m_playerStats.Diamonds));

	healthPointsText.SetText("Health : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Health)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Health) + m_HealthPointsToAdd))
	);

	staminaPointsText.SetText("Stamina : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Stamina)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Stamina) + m_StaminaPointsToAdd))
	);

	strengthPointsText.SetText("Strength : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Strength)))
		+ "  -->         "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Strength) + m_StrengthPointsToAdd))
	);

	defensePointsText.SetText("Defense : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Defense)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Defense) + m_DefensePointsToAdd))
	);

	magicPointsText.SetText("Magic : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Magic)))
		+ "  -->               "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Magic) + m_MagicPointsToAdd))
	);

	healthStatText.SetText("Health : " 
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Health)))
		+ "      -->   "
		+ std::to_string(static_cast<int>(m_playerAttributes.CalculateUpgradeValue(Attributes::Health)))
	);

	staminaStatText.SetText("Stamina : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Stamina)))
		+ "   -->   "
		+ std::to_string(static_cast<int>(m_playerAttributes.CalculateUpgradeValue(Attributes::Stamina)))
	);

	strengthStatText.SetText("Strength : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Strength)))
		+ "    -->   "
		+ std::to_string(static_cast<int>(m_playerAttributes.CalculateUpgradeValue(Attributes::Strength)))
	);

	defenseStatText.SetText("Defense : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Defense)))
		+ "     -->   "
		+ std::to_string(static_cast<int>(m_playerAttributes.CalculateUpgradeValue(Attributes::Defense)))
	);

	magicStatText.SetText("Magic : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttribute(Attributes::Magic)))
		+ "         -->   "
		+ std::to_string(static_cast<int>(m_playerAttributes.CalculateUpgradeValue(Attributes::Magic)))
	);

	speedStatText.SetText("Speed : "
		+ std::to_string(static_cast<int>(m_playerStats.Speed))
		+ "      -->   "
	);

	if (m_AttributePointsToSpend == m_playerAttributes.GetAttributePointsAvailable())
	{
		healthRightArrowButton.SetDisabled(true);
		staminaRightArrowButton.SetDisabled(true);
		strengthRightArrowButton.SetDisabled(true);
		defenseRightArrowButton.SetDisabled(true);
		magicRightArrowButton.SetDisabled(true);
	}
	else if (m_AttributePointsToSpend > 0)
	{
		healthRightArrowButton.SetDisabled(false);
		staminaRightArrowButton.SetDisabled(false);
		strengthRightArrowButton.SetDisabled(false);
		defenseRightArrowButton.SetDisabled(false);
		magicRightArrowButton.SetDisabled(false);
	}

	m_HealthPointsToAdd > 0 ? healthLeftArrowButton.SetDisabled(false) : healthLeftArrowButton.SetDisabled(true);
	m_StaminaPointsToAdd > 0 ? staminaLeftArrowButton.SetDisabled(false) : staminaLeftArrowButton.SetDisabled(true);;
	m_StrengthPointsToAdd > 0 ? strengthLeftArrowButton.SetDisabled(false) : strengthLeftArrowButton.SetDisabled(true);
	m_DefensePointsToAdd > 0 ? defenseLeftArrowButton.SetDisabled(false) : defenseLeftArrowButton.SetDisabled(true);
	m_MagicPointsToAdd > 0 ? magicLeftArrowButton.SetDisabled(false) : magicLeftArrowButton.SetDisabled(true);
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

	m_AttributePointsToSpend -= 1;
	m_HealthPointsToAdd -= 1;

	healthPointsText.SetText("Health : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Health)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Health) - m_HealthPointsToAdd))
	);

	healthRightArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnHealthRightArrowButtonClicked()
{
	GAME_INFO("Health Right Arrow Clicked");

	m_AttributePointsToSpend += 1;
	m_HealthPointsToAdd += 1;

	healthPointsText.SetText("Health : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Health)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Health) + m_HealthPointsToAdd))
	);

	healthLeftArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnStaminaLeftArrowButtonClicked()
{
	GAME_INFO("Stamina Left Arrow Clicked");

	m_AttributePointsToSpend -= 1;
	m_StaminaPointsToAdd -= 1;

	staminaPointsText.SetText("Stamina : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Stamina)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Stamina) - m_StaminaPointsToAdd))
	);

	staminaRightArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnStaminaRightArrowButtonClicked()
{
	GAME_INFO("Stamina Right Arrow Clicked");

	m_AttributePointsToSpend += 1;
	m_StaminaPointsToAdd += 1;

	staminaPointsText.SetText("Stamina : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Stamina)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Stamina) + m_StaminaPointsToAdd))
	);

	staminaLeftArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnStrengthLeftArrowButtonClicked()
{
	GAME_INFO("Strength Left Arrow Clicked");

	m_AttributePointsToSpend -= 1;
	m_StrengthPointsToAdd -= 1;

	strengthPointsText.SetText("Strength : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Strength)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Strength) - m_StrengthPointsToAdd))
	);

	strengthRightArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnStrengthRightArrowButtonClicked()
{
	GAME_INFO("Strength Right Arrow Clicked");

	m_AttributePointsToSpend += 1;
	m_StrengthPointsToAdd += 1;

	strengthPointsText.SetText("Strength : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Strength)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Strength) + m_StrengthPointsToAdd))
	);

	strengthLeftArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnDefenseLeftArrowButtonClicked()
{
	GAME_INFO("Defense Left Arrow Clicked");

	m_AttributePointsToSpend -= 1;
	m_DefensePointsToAdd -= 1;

	defensePointsText.SetText("Defense : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Defense)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Defense) - m_StrengthPointsToAdd))
	);

	defenseRightArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnDefenseRightArrowButtonClicked()
{
	GAME_INFO("Defense Right Arrow Clicked");

	m_AttributePointsToSpend += 1;
	m_DefensePointsToAdd += 1;

	defensePointsText.SetText("Defense : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Defense)))
		+ "  -->           "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Defense) + m_StrengthPointsToAdd))
	);

	defenseLeftArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnMagicLeftArrowButtonClicked()
{
	GAME_INFO("Magic Left Arrow Clicked");

	m_AttributePointsToSpend -= 1;
	m_MagicPointsToAdd -= 1;

	magicPointsText.SetText("Magic : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Magic)))
		+ "  -->               "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Magic) - m_MagicPointsToAdd))
	);

	magicRightArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnMagicRightArrowButtonClicked()
{
	GAME_INFO("Magic Right Arrow Clicked");

	m_AttributePointsToSpend += 1;
	m_MagicPointsToAdd += 1;

	magicPointsText.SetText("Magic : "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Magic)))
		+ "  -->               "
		+ std::to_string(static_cast<int>(m_playerAttributes.GetAttributePointsSpent(Attributes::Magic) + m_MagicPointsToAdd))
	);

	magicLeftArrowButton.SetDisabled(false);
}

void CharacterInfoMenu::OnConfirmAttributePointsButtonClicked()
{
	GAME_INFO("Confirm Attributes Button Clicked");
}

void CharacterInfoMenu::OnCloseButtonClicked()
{
	GAME_INFO("Close Button Clicked!");
	SetVisibility(false);
}

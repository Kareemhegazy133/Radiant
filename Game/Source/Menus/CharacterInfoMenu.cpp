#include "CharacterInfoMenu.h"

#include "Layers/GameLayer.h"

#include "GameTheme.h"

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
	characterIcon.SetPosition({ 190.f, 170.f });

	characterHealthText.SetText("800/1000");
	characterHealthText.SetFont(GameTheme::GetFont());
	characterHealthText.SetCharacterSize(25);
	characterHealthText.SetPosition({ 350.f, 170.f });
	characterHealthText.SetColor(GameTheme::GetLightTextColor());

	characterStaminaText.SetText("800/1000");
	characterStaminaText.SetFont(GameTheme::GetFont());
	characterStaminaText.SetCharacterSize(25);
	characterStaminaText.SetPosition({ 350.f, 200.f });
	characterStaminaText.SetColor(GameTheme::GetLightTextColor());

	coinsIcon.SetPosition({ 300.f, 243.f });
	characterCoinsText.SetText("20000");
	characterCoinsText.SetFont(GameTheme::GetFont());
	characterCoinsText.SetCharacterSize(25);
	characterCoinsText.SetPosition({ 335.f, 243.f });
	characterCoinsText.SetColor(GameTheme::GetLightTextColor());

	diamondsIcon.SetPosition({ 420.f, 243.f });
	characterDiamondsText.SetText("20000");
	characterDiamondsText.SetFont(GameTheme::GetFont());
	characterDiamondsText.SetCharacterSize(25);
	characterDiamondsText.SetPosition({ 455.f, 243.f });
	characterDiamondsText.SetColor(GameTheme::GetLightTextColor());

	characterLevelText.SetText("Level : 10");
	characterLevelText.SetFont(GameTheme::GetFont());
	characterLevelText.SetCharacterSize(21);
	characterLevelText.SetPosition({ 240.f, 295.f });
	characterLevelText.SetColor(GameTheme::GetDarkTextColor());

	characterLivesText.SetText("Lives : 4");
	characterLivesText.SetFont(GameTheme::GetFont());
	characterLivesText.SetCharacterSize(21);
	characterLivesText.SetPosition({ 400.f, 295.f });
	characterLivesText.SetColor(GameTheme::GetDarkTextColor());

	characterInfoTitleText.SetText("Character\n     Info");
	characterInfoTitleText.SetFont(GameTheme::GetFont());
	characterInfoTitleText.SetCharacterSize(34);
	characterInfoTitleText.SetPosition({ 775.f, 120.f });
	characterInfoTitleText.SetColor(GameTheme::GetLightTextColor());

	closeButton.SetPosition({ 1035.f, 190.f });
	closeButton.SetSize({ 80.f, 80.f });
	closeButton.SetButtonCallback(std::bind(&CharacterInfoMenu::OnCloseButtonClicked, this));
	closeButton.SetTexture(&TextureManager::GetTexture("CloseButton"));
	closeButton.SetHoveredTexture(&TextureManager::GetTexture("CloseButtonHovered"));

	healthStatText.SetText("Health : 1000");
	healthStatText.SetFont(GameTheme::GetFont());
	healthStatText.SetCharacterSize(21);
	healthStatText.SetPosition({ 785.f, 223.f });
	healthStatText.SetColor(GameTheme::GetLightTextColor());

	staminaStatText.SetText("Stamina : 100");
	staminaStatText.SetFont(GameTheme::GetFont());
	staminaStatText.SetCharacterSize(21);
	staminaStatText.SetPosition({ 785.f, 275.f });
	staminaStatText.SetColor(GameTheme::GetLightTextColor());

	strengthStatText.SetText("Strength : 100");
	strengthStatText.SetFont(GameTheme::GetFont());
	strengthStatText.SetCharacterSize(21);
	strengthStatText.SetPosition({ 785.f, 326.f });
	strengthStatText.SetColor(GameTheme::GetLightTextColor());

	defenseStatText.SetText("Defense : 100");
	defenseStatText.SetFont(GameTheme::GetFont());
	defenseStatText.SetCharacterSize(21);
	defenseStatText.SetPosition({ 785.f, 377.f });
	defenseStatText.SetColor(GameTheme::GetLightTextColor());

	magicStatText.SetText("Magic : 100");
	magicStatText.SetFont(GameTheme::GetFont());
	magicStatText.SetCharacterSize(21);
	magicStatText.SetPosition({ 785.f, 428.f });
	magicStatText.SetColor(GameTheme::GetLightTextColor());

	speedStatText.SetText("Speed : 1000");
	speedStatText.SetFont(GameTheme::GetFont());
	speedStatText.SetCharacterSize(21);
	speedStatText.SetPosition({ 785.f, 479.f });
	speedStatText.SetColor(GameTheme::GetLightTextColor());

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

void CharacterInfoMenu::OnCloseButtonClicked()
{
	GAME_INFO("Close Button Clicked!");
	SetVisibility(false);
}

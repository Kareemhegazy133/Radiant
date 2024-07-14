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

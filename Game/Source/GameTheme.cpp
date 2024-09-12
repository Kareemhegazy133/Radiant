#include "GameTheme.h"

// Definition of static members
sf::Font GameTheme::s_Font;

sf::Color GameTheme::s_LightTextColor;
sf::Color GameTheme::s_DarkTextColor;

sf::Color GameTheme::s_DisabledTextColor;

/**** Button Properties ****/
sf::Color GameTheme::s_ButtonTextColor;

unsigned int GameTheme::s_CommonButtonTextSize;
//unsigned int GameTheme::s_SmallButtonTextSize;
unsigned int GameTheme::s_MediumButtonTextSize;
unsigned int GameTheme::s_LargeButtonTextSize;

glm::vec2 GameTheme::s_CommonButtonSize;
//glm::vec2 GameTheme::s_SmallButtonSize;
glm::vec2 GameTheme::s_MediumButtonSize;
glm::vec2 GameTheme::s_LargeButtonSize;

void GameTheme::Initialize()
{
    s_Font = ResourceManager::GetFont("Euljiro");

    s_LightTextColor = sf::Color(102, 168, 191);
    s_DarkTextColor = sf::Color(7, 11, 19);

    s_DisabledTextColor = sf::Color(117, 158, 170);

    /**** Button Properties ****/
    s_ButtonTextColor = sf::Color(41, 77, 89);

    s_CommonButtonTextSize = 21;
    //s_SmallButtonTextSize = ;
    s_MediumButtonTextSize = 42;
    s_LargeButtonTextSize = 55;

    s_CommonButtonSize = { 150.f, 45.f };
    //s_SmallButtonSize = { .f, .f };
    s_MediumButtonSize = { 280.f, 90.f };
    s_LargeButtonSize = { 340.f, 90.f };
}

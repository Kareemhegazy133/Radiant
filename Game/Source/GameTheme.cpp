#include "GameTheme.h"

// Definition of static members
sf::Font GameTheme::s_Font;

sf::Color GameTheme::s_LightTextColor;
sf::Color GameTheme::s_DarkTextColor;

/**** Button Properties ****/
sf::Color GameTheme::s_ButtonTextColor;

//unsigned int GameTheme::s_SmallButtonTextSize;
unsigned int GameTheme::s_MediumButtonTextSize;
unsigned int GameTheme::s_LargeButtonTextSize;

//sf::Vector2f GameTheme::s_SmallButtonSize;
sf::Vector2f GameTheme::s_MediumButtonSize;
sf::Vector2f GameTheme::s_LargeButtonSize;

void GameTheme::Initialize()
{
    s_Font = FontManager::GetFont("Euljiro");

    s_LightTextColor = sf::Color(102, 168, 191);
    s_DarkTextColor = sf::Color(7, 11, 19);

    /**** Button Properties ****/
    s_ButtonTextColor = sf::Color(41, 77, 89);

    //s_SmallButtonTextSize = ;
    s_MediumButtonTextSize = 42;
    s_LargeButtonTextSize = 55;

    //s_SmallButtonSize = { .f, .f };
    s_MediumButtonSize = { 280.f, 90.f };
    s_LargeButtonSize = { 340.f, 90.f };
}

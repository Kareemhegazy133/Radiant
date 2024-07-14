#pragma once

#include <Engine.h>

using namespace Engine;

class GameTheme
{
public:
    static void Initialize();

    inline static const sf::Font& GetFont() { return s_Font; }

    inline static const sf::Color& GetLightTextColor() { return s_LightTextColor; }
    inline static const sf::Color& GetDarkTextColor() { return s_DarkTextColor; }

    /**** Button Properties ****/
    inline static const sf::Color& GetButtonTextColor() { return s_ButtonTextColor; }

    //inline static unsigned int GetSmallButtonTextSize() { return s_SmallButtonTextSize; }
    inline static unsigned int GetMediumButtonTextSize() { return s_MediumButtonTextSize; }
    inline static unsigned int GetLargeButtonTextSize() { return s_LargeButtonTextSize; }

    //inline static const sf::Vector2f& GetSmallButtonSize() { return s_SmallButtonSize; }
    inline static const sf::Vector2f& GetMediumButtonSize() { return s_MediumButtonSize; }
    inline static const sf::Vector2f& GetLargeButtonSize() { return s_LargeButtonSize; }

    //inline static const sf::Texture* GetSmallButtonNormalTexture() { return &TextureManager::GetTexture("SmallButton"); }
    inline static const sf::Texture* GetMediumButtonNormalTexture() { return &TextureManager::GetTexture("MediumButton"); }
    inline static const sf::Texture* GetLargeButtonNormalTexture() { return &TextureManager::GetTexture("LargeButton"); }

private:
    static sf::Font s_Font;

    static sf::Color s_LightTextColor;
    static sf::Color s_DarkTextColor;

    /**** Button Properties ****/
    static sf::Color s_ButtonTextColor;

    //static unsigned int s_SmallButtonTextSize;
    static unsigned int s_MediumButtonTextSize;
    static unsigned int s_LargeButtonTextSize;

    //static sf::Vector2f s_SmallButtonSize;
    static sf::Vector2f s_MediumButtonSize;
    static sf::Vector2f s_LargeButtonSize;
};

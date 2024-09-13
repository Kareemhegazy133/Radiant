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

    inline static const sf::Color& GetDisabledTextColor() { return s_DisabledTextColor; }

    /**** Button Properties ****/
    inline static const sf::Color& GetButtonTextColor() { return s_ButtonTextColor; }

    inline static unsigned int GetCommonButtonTextSize() { return s_CommonButtonTextSize; }
    //inline static unsigned int GetSmallButtonTextSize() { return s_SmallButtonTextSize; }
    inline static unsigned int GetMediumButtonTextSize() { return s_MediumButtonTextSize; }
    inline static unsigned int GetLargeButtonTextSize() { return s_LargeButtonTextSize; }

    inline static const glm::vec2& GetCommonButtonSize() { return s_CommonButtonSize; }
    //inline static const sf::Vector2f& GetSmallButtonSize() { return s_SmallButtonSize; }
    inline static const glm::vec2& GetMediumButtonSize() { return s_MediumButtonSize; }
    inline static const glm::vec2& GetLargeButtonSize() { return s_LargeButtonSize; }

    inline static const sf::Texture* GetCommonButtonNormalTexture() { return &ResourceManager::GetTexture("CommonButton"); }
    inline static const sf::Texture* GetCommonButtonHoveredTexture() { return &ResourceManager::GetTexture("CommonButtonHovered"); }
    inline static const sf::Texture* GetCommonButtonDisabledTexture() { return &ResourceManager::GetTexture("CommonButtonDisabled"); }
    //inline static const sf::Texture* GetSmallButtonNormalTexture() { return &ResourceManager::GetTexture("SmallButton"); }
    inline static const sf::Texture* GetMediumButtonNormalTexture() { return &ResourceManager::GetTexture("MediumButton"); }
    inline static const sf::Texture* GetLargeButtonNormalTexture() { return &ResourceManager::GetTexture("LargeButton"); }

private:
    static sf::Font s_Font;

    static sf::Color s_LightTextColor;
    static sf::Color s_DarkTextColor;

    static sf::Color s_DisabledTextColor;

    /**** Button Properties ****/
    static sf::Color s_ButtonTextColor;

    static unsigned int s_CommonButtonTextSize;
    //static unsigned int s_SmallButtonTextSize;
    static unsigned int s_MediumButtonTextSize;
    static unsigned int s_LargeButtonTextSize;

    static glm::vec2 s_CommonButtonSize;
    //static glm::vec2 s_SmallButtonSize;
    static glm::vec2 s_MediumButtonSize;
    static glm::vec2 s_LargeButtonSize;
};

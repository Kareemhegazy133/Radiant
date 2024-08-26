#pragma once

#include <SFML/Graphics/Font.hpp>

namespace Engine {

    class FontManager {
    public:
        FontManager();
        ~FontManager();

        inline static bool LoadFont(const std::string& name, const std::string& filePath)
        {
            return s_Instance->LoadFontInternal(name, filePath);
        }

        inline static const sf::Font& GetFont(const std::string& name)
        {
            return s_Instance->GetFontInternal(name);
        }

        // Disable copy constructor and assignment operator
        FontManager(const FontManager&) = delete;
        FontManager& operator=(const FontManager&) = delete;

    private:
        bool LoadFontInternal(const std::string& name, const std::string& filePath);
        const sf::Font& GetFontInternal(const std::string& name) const;
    private:
        static FontManager* s_Instance;
        std::unordered_map<std::string, sf::Font> m_Fonts;
    };
}
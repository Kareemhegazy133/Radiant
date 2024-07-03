#include "Enginepch.h"

#include "FontManager.h"

namespace Engine {

	FontManager* FontManager::s_Instance = nullptr;

	FontManager::FontManager()
	{
		ENGINE_ASSERT(!s_Instance, "FontManager already exists!");
		s_Instance = this;
	}

	FontManager::~FontManager()
	{
	}

    bool FontManager::LoadFontInternal(const std::string& name, const std::string& filePath)
    {
        sf::Font font;
        if (font.loadFromFile(filePath))
        {
            m_Fonts[name] = font;
            return true;
        }
        return false;
    }

    const sf::Font& FontManager::GetFontInternal(const std::string& name) const
    {
        auto it = m_Fonts.find(name);
        if (it != m_Fonts.end())
        {
            return it->second;
        }
        // TODO: Default font maybe?

        /*
        static sf::Font defaultFont;
        static bool isDefaultFontLoaded = false;
        if (!isDefaultFontLoaded)
        {
            if (!defaultFont.loadFromFile("Assets/Font/Default.otf"))
            {
                ENGINE_ERROR("Default font not found!");
            }
            isDefaultFontLoaded = true;
        }

        ENGINE_ERROR("Font not found: {0}", name);
        return defaultFont;
        */
        ENGINE_ERROR("Font not found: {0}", name);
        return sf::Font();
    }
}


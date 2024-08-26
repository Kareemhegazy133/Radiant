#include "Enginepch.h"

#include "ResourceManager.h"

namespace Engine {

    static ResourceManager* s_Instance = nullptr;

    void ResourceManager::Init()
    {
        ENGINE_ASSERT(!s_Instance);
        s_Instance = new ResourceManager();

        ENGINE_INFO("Resource Manager Init");
    }

    void ResourceManager::Shutdown()
    {
        delete s_Instance;
        s_Instance = nullptr;
        ENGINE_INFO("Resource Manager Shutdown");
    }

    bool ResourceManager::LoadTexture(const std::string& identifier, const std::string& filePath)
    {
        ENGINE_ASSERT(s_Instance);
        sf::Texture texture;
        if (!texture.loadFromFile(filePath)) {
            ENGINE_ERROR("Failed to load texture from file: {0}", filePath);
            return false;
        }
        s_Instance->m_Textures[identifier] = texture;
        return true;
    }

    const sf::Texture& ResourceManager::GetTexture(const std::string& identifier)
    {
        ENGINE_ASSERT(s_Instance);
        auto it = s_Instance->m_Textures.find(identifier);
        if (it != s_Instance->m_Textures.end()) {
            return it->second;
        }
        ENGINE_ERROR("Texture with identifier '{0}' not found", identifier);
        // Return a default texture
        return s_Instance->m_Textures.at("default");
    }

    bool ResourceManager::IsTextureLoaded(const std::string& identifier)
    {
        ENGINE_ASSERT(s_Instance);
        return s_Instance->m_Textures.find(identifier) != s_Instance->m_Textures.end();
    }

    void ResourceManager::UnloadTexture(const std::string& identifier)
    {
        ENGINE_ASSERT(s_Instance);
        auto it = s_Instance->m_Textures.find(identifier);
        if (it != s_Instance->m_Textures.end()) {
            s_Instance->m_Textures.erase(it);
        }
    }

    bool ResourceManager::LoadFont(const std::string& name, const std::string& filePath)
    {
        ENGINE_ASSERT(s_Instance);
        sf::Font font;
        if (font.loadFromFile(filePath))
        {
            s_Instance->m_Fonts[name] = font;
            return true;
        }
        return false;
    }

    const sf::Font& ResourceManager::GetFont(const std::string& name)
    {
        ENGINE_ASSERT(s_Instance);
        auto it = s_Instance->m_Fonts.find(name);
        if (it != s_Instance->m_Fonts.end())
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
        ENGINE_ASSERT(false);
        return sf::Font();
    }

}


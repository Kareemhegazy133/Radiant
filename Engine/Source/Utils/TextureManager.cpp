#include "Enginepch.h"
#include "TextureManager.h"

namespace Engine {

    TextureManager* TextureManager::s_Instance = nullptr;

    TextureManager::TextureManager()
    {
        ENGINE_ASSERT(!s_Instance, "TextureManager already exists!");
        s_Instance = this;
    }

    TextureManager::~TextureManager()
    {

    }

    bool TextureManager::loadTexture(const std::string& identifier, const std::string& filePath) {
        sf::Texture texture;
        if (!texture.loadFromFile(filePath)) {
            ENGINE_ERROR("Failed to load texture from file: {0}", filePath);
            return false;
        }
        m_Textures[identifier] = texture;
        return true;
    }

    const sf::Texture& TextureManager::getTexture(const std::string& identifier) const {
        auto it = m_Textures.find(identifier);
        if (it != m_Textures.end()) {
            return it->second;
        }
        ENGINE_ERROR("Texture with identifier '{0}' not found", identifier);
        // Return a default texture
        return m_Textures.at("default");
    }

    bool TextureManager::isTextureLoaded(const std::string& identifier) const {
        return m_Textures.find(identifier) != m_Textures.end();
    }

    void TextureManager::unloadTexture(const std::string& identifier) {
        auto it = m_Textures.find(identifier);
        if (it != m_Textures.end()) {
            m_Textures.erase(it);
        }
    }

}

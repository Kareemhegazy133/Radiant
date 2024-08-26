#pragma once

#include <SFML/Graphics.hpp>

namespace Engine {

    class TextureManager {
    public:
        TextureManager();
        ~TextureManager();

        inline static bool LoadTexture(const std::string& identifier, const std::string& filePath)
        {
            return s_Instance->LoadTextureInternal(identifier, filePath);
        }

        inline static const sf::Texture& GetTexture(const std::string& identifier)
        {
            return s_Instance->GetTextureInternal(identifier);
        }

        inline static bool IsTextureLoaded(const std::string& identifier)
        {
            return s_Instance->IsTextureLoadedInternal(identifier);
        }

        inline static void UnloadTexture(const std::string& identifier)
        {
            return s_Instance->UnloadTextureInternal(identifier);
        }

        // Disable copy constructor and assignment operator
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

    private:
        // Load texture from file and associate it with a given identifier
        bool LoadTextureInternal(const std::string& identifier, const std::string& filePath);

        // Get a texture by its identifier
        const sf::Texture& GetTextureInternal(const std::string& identifier) const;

        // Check if a texture is loaded
        bool IsTextureLoadedInternal(const std::string& identifier) const;

        // Unload a texture by its identifier
        void UnloadTextureInternal(const std::string& identifier);

    private:
        static TextureManager* s_Instance;
        std::unordered_map<std::string, sf::Texture> m_Textures;
    };
}
#pragma once

#include <SFML/Graphics.hpp>
#include "Enginepch.h"

namespace Engine {

    class TextureManager {
    public:
        TextureManager();
        ~TextureManager();

        // Load texture from file and associate it with a given identifier
        bool loadTexture(const std::string& identifier, const std::string& filePath);

        // Get a texture by its identifier
        const sf::Texture& getTexture(const std::string& identifier) const;

        // Check if a texture is loaded
        bool isTextureLoaded(const std::string& identifier) const;

        // Unload a texture by its identifier
        void unloadTexture(const std::string& identifier);

        inline static TextureManager& Get() { return *s_Instance; }

    private:
        static TextureManager* s_Instance;
        std::unordered_map<std::string, sf::Texture> m_Textures;
    };
}
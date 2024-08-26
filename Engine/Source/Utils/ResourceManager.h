#pragma once

#include <SFML/Graphics/Font.hpp>

namespace Engine {

    class ResourceManager {
    public:
        static void Init();
        static void Shutdown();

        // Texture Methods
        static bool LoadTexture(const std::string& identifier, const std::string& filePath);
        static const sf::Texture& GetTexture(const std::string& identifier);
        static bool IsTextureLoaded(const std::string& identifier);
        static void UnloadTexture(const std::string& identifier);

        // Font Methods
        static bool LoadFont(const std::string& name, const std::string& filePath);
        static const sf::Font& GetFont(const std::string& name);

        // Disable copy constructor and assignment operator
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

    private:

        std::unordered_map<std::string, sf::Texture> m_Textures;
        std::unordered_map<std::string, sf::Font> m_Fonts;
    };
}
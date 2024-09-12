#pragma once

#include <SFML/Graphics/Sprite.hpp>

namespace Engine {

    class SpriteComponent
    {
    public:

        SpriteComponent();
        SpriteComponent(const std::string& textureIdentifier);

        // Function to set the texture identifier and update the texture of the sprite
        void SetTextureIdentifier(const std::string& textureIdentifier);
        // Function to get the texture identifier
        const std::string& GetTextureIdentifier() const;

        // Function to get the size of the texture
        glm::vec2 GetTextureSize() const;

        void SetTextureRect(const glm::ivec4& rectangle);
        glm::ivec4 GetTextureRect() const;

        void SetPosition(const glm::vec2& position);
        void SetPosition(float x, float y);

        void SetRotation(float angle);

        void SetScale(const glm::vec2& scale);
        void SetScale(float factorX, float factorY);

        void SetOrigin(const glm::vec2& origin);
        void SetOrigin(float originX, float originY);

        glm::vec2 GetScale() const;

        operator const sf::Sprite& () { return Sprite; }
        operator sf::Sprite& () { return Sprite; }

    private:
        sf::Sprite Sprite;
        // Identifier for the texture in the ResourceManager
        std::string TextureIdentifier;

    };
}
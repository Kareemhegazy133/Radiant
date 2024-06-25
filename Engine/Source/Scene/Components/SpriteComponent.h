#pragma once

#include "SFML/Graphics/Sprite.hpp"

#include "Scene/Components/Component.h"
#include "Scene/Components/AnimationComponent.h"

namespace Engine {

    class SpriteComponent : public Component
    {
    public:

        SpriteComponent();
        SpriteComponent(const std::string& textureIdentifier, AnimationComponent* animation = nullptr);

        // Function to set the texture identifier and update the texture of the sprite
        void SetTextureIdentifier(const std::string& textureIdentifier);
        // Function to get the texture identifier
        const std::string& GetTextureIdentifier() const;

        // Function to get the size of the texture
        sf::Vector2f GetTextureSize() const;

        void SetTextureRect(const sf::IntRect& rectangle);
        const sf::IntRect& GetTextureRect() const;

        void SetPosition(const sf::Vector2f& position);
        void SetPosition(float x, float y);

        void SetRotation(float angle);

        void SetScale(const sf::Vector2f& scale);
        void SetScale(float factorX, float factorY);

        // Update the sprite's animation
        void Update(Timestep ts);

        operator const sf::Sprite& () { return Sprite; }
        operator sf::Sprite& () { return Sprite; }

    public:
        // Pointer to an AnimationComponent (optional)
        AnimationComponent* Animation;
    private:
        sf::Sprite Sprite;
        // Identifier for the texture in the TextureManager
        std::string TextureIdentifier;

    };
}
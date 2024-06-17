#pragma once

#include "SFML/Graphics/Sprite.hpp"
#include "Utils/TextureManager.h"

#include "World/Components/Component.h"
#include "World/Components/AnimationComponent.h"

#include "Enginepch.h"

namespace Engine {

    class SpriteComponent : public Component
    {
    public:
        sf::Sprite Sprite;
        std::string TextureIdentifier; // Identifier for the texture in the TextureManager
        AnimationComponent* Animation; // Pointer to an AnimationComponent (optional)

        SpriteComponent() : Animation(nullptr) {}

        SpriteComponent(const std::string& textureIdentifier, AnimationComponent* animation = nullptr)
            : Sprite(), TextureIdentifier(textureIdentifier), Animation(animation)
        {
            // Set the texture using the TextureManager
            Sprite.setTexture(TextureManager::Get().getTexture(textureIdentifier));

            // If there's an animation, set the initial texture rectangle
            if (Animation) {
                Sprite.setTextureRect(Animation->GetCurrentFrame());
            }
        }

        operator const sf::Sprite& () { return Sprite; }

        // Function to set the texture identifier and update the texture of the sprite
        void setTextureIdentifier(const std::string& textureIdentifier) {
            TextureIdentifier = textureIdentifier;
            Sprite.setTexture(TextureManager::Get().getTexture(textureIdentifier));
        }

        // Function to get the texture identifier
        const std::string& getTextureIdentifier() const {
            return TextureIdentifier;
        }

        // Function to get the size of the texture
        sf::Vector2f getTextureSize() const {
            sf::Vector2i size;
            if (Animation)
            {
                size = Animation->GetCurrentFrameSize();
            }
            else {
                size = Sprite.getTextureRect().getSize();
            }
            return sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y));
        }

        void setTextureRect(const sf::IntRect& rectangle) {
            Sprite.setTextureRect(rectangle);
        }

        const sf::IntRect& getTextureRect() const {
            return Sprite.getTextureRect();
        }

        void setPosition(const sf::Vector2f& position) {
            Sprite.setPosition(position);
        }

        void setPosition(float x, float y) {
            Sprite.setPosition(x, y);
        }

        void setRotation(float angle) {
            Sprite.setRotation(angle);
        }

        void setScale(const sf::Vector2f& scale) {
            Sprite.setScale(scale);
        }

        void setScale(float factorX, float factorY) {
            Sprite.setScale(factorX, factorY);
        }

        // Update the sprite's animation
        void update(Timestep ts) {
            if (Animation) {
                Animation->Update(ts);
                Sprite.setTextureRect(Animation->GetCurrentFrame());
            }
        }
    };
}
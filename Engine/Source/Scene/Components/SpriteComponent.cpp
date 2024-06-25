#include "Enginepch.h"

#include "SpriteComponent.h"

#include "Resources/TextureManager.h"

namespace Engine {

    SpriteComponent::SpriteComponent()
        : Animation(nullptr)
    {
    }

    SpriteComponent::SpriteComponent(const std::string& textureIdentifier, AnimationComponent* animation)
        : Sprite(), TextureIdentifier(textureIdentifier), Animation(animation)
    {
        // Set the texture using the TextureManager
        Sprite.setTexture(TextureManager::Get().getTexture(textureIdentifier));

        // If there's an animation, set the initial texture rectangle
        if (Animation) {
            Sprite.setTextureRect(Animation->GetCurrentFrame());
        }
    }

    void SpriteComponent::SetTextureIdentifier(const std::string& textureIdentifier)
    {
        TextureIdentifier = textureIdentifier;
        Sprite.setTexture(TextureManager::Get().getTexture(textureIdentifier));
    }

    const std::string& SpriteComponent::GetTextureIdentifier() const
    {
        return TextureIdentifier;
    }

    sf::Vector2f SpriteComponent::GetTextureSize() const
    {
        sf::Vector2i size;
        if (Animation) {
            size = Animation->GetCurrentFrameSize();
        }
        else 
        {
            size = Sprite.getTextureRect().getSize();
        }
        return sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y));
    }

    void SpriteComponent::SetTextureRect(const sf::IntRect& rectangle)
    {
        Sprite.setTextureRect(rectangle);
    }

    const sf::IntRect& SpriteComponent::GetTextureRect() const
    {
        return Sprite.getTextureRect();
    }

    void SpriteComponent::SetPosition(const sf::Vector2f& position)
    {
        Sprite.setPosition(position);
    }

    void SpriteComponent::SetPosition(float x, float y)
    {
        Sprite.setPosition(x, y);
    }

    void SpriteComponent::SetRotation(float angle)
    {
        Sprite.setRotation(angle);
    }

    void SpriteComponent::SetScale(const sf::Vector2f& scale)
    {
        Sprite.setScale(scale);
    }

    void SpriteComponent::SetScale(float factorX, float factorY)
    {
        Sprite.setScale(factorX, factorY);
    }

    void SpriteComponent::Update(Timestep ts)
    {
        if (Animation)
        {
            Animation->Update(ts);
            Sprite.setTextureRect(Animation->GetCurrentFrame());
        }
    }
}

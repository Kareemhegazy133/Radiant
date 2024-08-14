#include "Enginepch.h"

#include "SpriteComponent.h"

#include "Resources/TextureManager.h"

namespace Engine {

    SpriteComponent::SpriteComponent()
    {
    }

    SpriteComponent::SpriteComponent(const std::string& textureIdentifier)
        : Sprite(), TextureIdentifier(textureIdentifier)
    {
        Sprite.setTexture(TextureManager::GetTexture(textureIdentifier));
        Sprite.setOrigin(GetTextureSize() / 2.f);
    }

    void SpriteComponent::SetTextureIdentifier(const std::string& textureIdentifier)
    {
        TextureIdentifier = textureIdentifier;
        Sprite.setTexture(TextureManager::GetTexture(textureIdentifier));
        Sprite.setOrigin(GetTextureSize() / 2.f);
    }

    const std::string& SpriteComponent::GetTextureIdentifier() const
    {
        return TextureIdentifier;
    }

    sf::Vector2f SpriteComponent::GetTextureSize() const
    {
        sf::Vector2i size = GetTextureRect().getSize();
        return sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y));
    }

    void SpriteComponent::SetTextureRect(const sf::IntRect& rectangle)
    {
        Sprite.setTextureRect(rectangle);
        Sprite.setOrigin(GetTextureSize() / 2.f);
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

    void SpriteComponent::SetOrigin(const sf::Vector2f& origin)
    {
        Sprite.setOrigin(origin);
    }

    void SpriteComponent::SetOrigin(float originX, float originY)
    {
        Sprite.setOrigin(originX, originY);
    }

    const sf::Vector2f& SpriteComponent::GetScale() const
    {
        return Sprite.getScale();
    }

}

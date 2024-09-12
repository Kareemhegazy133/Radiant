#include "Enginepch.h"

#include "SpriteComponent.h"
#include "Utils/ResourceManager.h"

namespace Engine {

    SpriteComponent::SpriteComponent()
    {
    }

    SpriteComponent::SpriteComponent(const std::string& textureIdentifier)
        : Sprite(), TextureIdentifier(textureIdentifier)
    {
        Sprite.setTexture(ResourceManager::GetTexture(textureIdentifier));
        SetOrigin(GetTextureSize() / 2.0f);
    }

    void SpriteComponent::SetTextureIdentifier(const std::string& textureIdentifier)
    {
        TextureIdentifier = textureIdentifier;
        Sprite.setTexture(ResourceManager::GetTexture(textureIdentifier));
        SetOrigin(GetTextureSize() / 2.0f);
    }

    const std::string& SpriteComponent::GetTextureIdentifier() const
    {
        return TextureIdentifier;
    }

    glm::vec2 SpriteComponent::GetTextureSize() const
    {
        sf::Vector2u size = Sprite.getTexture()->getSize();
        return glm::vec2(static_cast<float>(size.x), static_cast<float>(size.y));
    }

    void SpriteComponent::SetTextureRect(const glm::ivec4& rectangle)
    {
        Sprite.setTextureRect(sf::IntRect(rectangle.x, rectangle.y, rectangle.z, rectangle.w));
        SetOrigin(GetTextureSize() / 2.0f);
    }

    glm::ivec4 SpriteComponent::GetTextureRect() const
    {
        sf::IntRect rect = Sprite.getTextureRect();
        return glm::ivec4(rect.left, rect.top, rect.width, rect.height);
    }

    void SpriteComponent::SetPosition(const glm::vec2& position)
    {
        Sprite.setPosition(position.x, position.y);
    }

    void SpriteComponent::SetPosition(float x, float y)
    {
        Sprite.setPosition(x, y);
    }

    void SpriteComponent::SetRotation(float angle)
    {
        Sprite.setRotation(angle);
    }

    void SpriteComponent::SetScale(const glm::vec2& scale)
    {
        Sprite.setScale(scale.x, scale.y);
    }

    void SpriteComponent::SetScale(float factorX, float factorY)
    {
        Sprite.setScale(factorX, factorY);
    }

    void SpriteComponent::SetOrigin(const glm::vec2& origin)
    {
        Sprite.setOrigin(origin.x, origin.y);
    }

    void SpriteComponent::SetOrigin(float originX, float originY)
    {
        Sprite.setOrigin(originX, originY);
    }

    glm::vec2 SpriteComponent::GetScale() const
    {
        sf::Vector2f scale = Sprite.getScale();
        return glm::vec2(scale.x, scale.y);
    }

}

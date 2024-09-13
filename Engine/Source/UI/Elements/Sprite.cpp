#include "Enginepch.h"

#include "Sprite.h"

namespace Engine {

    Sprite::Sprite(const sf::Texture& texture, const glm::vec2& position)
        : m_Sprite(texture)
    {
        SetPosition(position);
    }

    void Sprite::SetTexture(const sf::Texture& texture)
    {
        m_Sprite.setTexture(texture);
    }

    void Sprite::SetPosition(const glm::vec2& position)
    {
        m_Sprite.setPosition(position.x, position.y);
    }

    void Sprite::SetScale(const glm::vec2& factors)
    {
        m_Sprite.setScale(factors.x, factors.y);
    }

    void Sprite::SetScale(float factorX, float factorY)
    {
        m_Sprite.setScale(factorX, factorY);
    }

    void Sprite::OnRender(sf::RenderWindow* renderWindow)
    {
        renderWindow->draw(m_Sprite);
    }

}

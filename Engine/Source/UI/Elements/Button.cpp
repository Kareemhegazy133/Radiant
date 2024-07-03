#include "Enginepch.h"

#include "Button.h"

namespace Engine {

    Button::Button(const sf::Vector2f& position, const sf::Vector2f& size)
    {
        SetSize(size);
        SetPosition(position);
    }

    void Button::SetSize(const sf::Vector2f& size)
    {
        m_Rectangle.setSize(size);
    }

    const sf::Vector2f& Button::GetSize() const
    {
        return m_Rectangle.getSize();
    }

    void Button::SetPosition(const sf::Vector2f& position)
    {
        m_Rectangle.setPosition(position);
    }

    void Button::SetPosition(float x, float y)
    {
        m_Rectangle.setPosition(x, y);
    }

    const sf::Vector2f& Button::GetPosition() const
    {
        return m_Rectangle.getPosition();
    }

    void Button::SetButtonCallback(std::function<void()> callback)
    {
        m_OnClick = callback;
    }

    void Button::SetTexture(const sf::Texture* texture)
    {
        m_Rectangle.setTexture(texture);
    }

    void Button::SetFillColor(const sf::Color& color)
    {
        m_Rectangle.setFillColor(color);
    }

    void Button::SetOutlineColor(const sf::Color& color)
    {
        m_Rectangle.setOutlineColor(color);
    }

    void Button::SetOutlineThickness(float thickness)
    {
        m_Rectangle.setOutlineThickness(thickness);
    }

    bool Button::IsHovered(const sf::Vector2i& mousePos) const
    {
        return m_Rectangle.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
    }

    void Button::OnClick()
    {
        if (m_OnClick)
        {
            m_OnClick();
        }
    }

}

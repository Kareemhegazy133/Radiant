#include "Enginepch.h"

#include "ButtonComponent.h"

namespace Engine {

    ButtonComponent::ButtonComponent(const sf::Vector2f& position, const sf::Vector2f& size, std::function<void()> onClick)
        : m_OnClick(onClick)
    {
        SetSize(size);
        SetPosition(position);
    }

    void ButtonComponent::SetSize(const sf::Vector2f& size)
    {
        m_Rectangle.setSize(size);
    }

    const sf::Vector2f& ButtonComponent::GetSize() const
    {
        return m_Rectangle.getSize();
    }

    void ButtonComponent::SetPosition(const sf::Vector2f& position)
    {
        m_Rectangle.setPosition(position);
    }

    void ButtonComponent::SetPosition(float x, float y)
    {
        m_Rectangle.setPosition(x, y);
    }

    const sf::Vector2f& ButtonComponent::GetPosition() const
    {
        return m_Rectangle.getPosition();
    }

    void ButtonComponent::SetFillColor(const sf::Color& color)
    {
        m_Rectangle.setFillColor(color);
    }

    void ButtonComponent::SetOutlineColor(const sf::Color& color)
    {
        m_Rectangle.setOutlineColor(color);
    }

    void ButtonComponent::SetOutlineThickness(float thickness)
    {
        m_Rectangle.setOutlineThickness(thickness);
    }

    bool ButtonComponent::IsHovered(const sf::Vector2i& mousePos) const
    {
        return m_Rectangle.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
    }

    void ButtonComponent::OnClick()
    {
        m_OnClick();
    }

}

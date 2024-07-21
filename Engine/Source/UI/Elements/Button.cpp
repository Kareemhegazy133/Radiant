#include "Enginepch.h"

#include "Button.h"

#include "Resources/TextureManager.h"

namespace Engine {

    Button::Button(const sf::Vector2f& position, const sf::Vector2f& size)
    {
        SetSize(size);
        SetPosition(position);
    }

    void Button::SetSize(const sf::Vector2f& size)
    {
        m_Rectangle.setSize(size);

        switch (static_cast<int>(size.x))
        {
        case 220:
            m_ButtonSize = ButtonSize::Common;
            break;
        /*case  :
        * m_ButtonSize = ButtonSize::Small;
            break;*/
        case 280:
            m_ButtonSize = ButtonSize::Medium;
            break;
        case 340:
            m_ButtonSize = ButtonSize::Large;
            break;
        default:
            m_ButtonSize = ButtonSize::Custom;
            break;
        }
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

    void Button::SetDisabled(bool disable)
    {
        m_IsDisabled = disable;
    }

    void Button::SetButtonCallback(std::function<void()> callback)
    {
        m_OnClick = callback;
    }

    void Button::SetTexture(const sf::Texture* texture)
    {
        m_Rectangle.setTexture(texture);
    }

    void Button::SetHoveredTexture(const sf::Texture* texture)
    {
        m_HoveredTexture = texture;
        // Store the normal texture
        m_NormalTexture = m_Rectangle.getTexture();
    }

    void Button::SetDisabledTexture(const sf::Texture* texture)
    {
        m_DisabledTexture = texture;
    }

    void Button::SetText(Text& textElement)
    {
        m_ButtonText = textElement;
        // Center the text to the button
        sf::FloatRect textBounds = m_ButtonText.GetLocalBounds();
        sf::FloatRect buttonBounds = m_Rectangle.getLocalBounds();
        m_ButtonText.SetOrigin({ textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f });
        m_ButtonText.SetPosition({ m_Rectangle.getPosition().x + buttonBounds.width / 2.0f, m_Rectangle.getPosition().y + buttonBounds.height / 2.0f });
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

    sf::FloatRect Button::GetLocalBounds()
    {
        return m_Rectangle.getLocalBounds();
    }

    bool Button::IsHovered(const sf::Vector2i& mousePos) const
    {
        return m_Rectangle.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
    }

    bool Button::IsDisabled() const
    {
        return m_IsDisabled;
    }

    void Button::OnClick()
    {
        if (!m_IsDisabled && m_OnClick)
        {
            m_OnClick();
        }
    }

    void Button::OnRender(sf::RenderWindow* renderWindow)
    {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*renderWindow);
        bool isCurrentlyHovered = IsHovered(mousePos);

        // if button is disabled then set its disabled texture
        if (IsDisabled())
        {
            switch (m_ButtonSize)
            {
            case ButtonSize::Common:
                SetTexture(&TextureManager::GetTexture("CommonButtonDisabled"));
                break;
            case ButtonSize::Small:
                //SetTexture(&TextureManager::GetTexture("SmallButtonDisabled"));
                break;
            case ButtonSize::Medium:
                //SetTexture(&TextureManager::GetTexture("MediumButtonDisabled"));
                break;
            case ButtonSize::Large:
                //SetTexture(&TextureManager::GetTexture("LargeButtonDisabled"));
                break;
            case ButtonSize::Custom:
                SetTexture(m_DisabledTexture);
                break;
            }
        }
        // if is now hovered and wasnt hovered last frame
        else if (isCurrentlyHovered && !m_IsHovered)
        {
            m_IsHovered = true;
            switch (m_ButtonSize)
            {
            case ButtonSize::Common:
                SetTexture(&TextureManager::GetTexture("CommonButtonHovered"));
                break;
            case ButtonSize::Small:
                //SetTexture(&TextureManager::GetTexture("SmallButtonHovered"));
                break;
            case ButtonSize::Medium:
                SetTexture(&TextureManager::GetTexture("MediumButtonHovered"));
                break;
            case ButtonSize::Large:
                SetTexture(&TextureManager::GetTexture("LargeButtonHovered"));
                break;
            case ButtonSize::Custom:
                SetTexture(m_HoveredTexture);
                break;
            }
            
        }
        // else if is now not hovered and was hovered last frame
        else if (!isCurrentlyHovered && m_IsHovered)
        {
            m_IsHovered = false;
            switch (m_ButtonSize)
            {
            case ButtonSize::Common:
                SetTexture(&TextureManager::GetTexture("CommonButton"));
                break;
            case ButtonSize::Small:
                //SetTexture(&TextureManager::GetTexture("SmallButton"));
                break;
            case ButtonSize::Medium:
                SetTexture(&TextureManager::GetTexture("MediumButton"));
                break;
            case ButtonSize::Large:
                SetTexture(&TextureManager::GetTexture("LargeButton"));
                break;
            case ButtonSize::Custom:
                SetTexture(m_NormalTexture);
                break;
            }
        }
        renderWindow->draw(m_Rectangle);
        renderWindow->draw(m_ButtonText.GetDrawable());
    }

}

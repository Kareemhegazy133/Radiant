#include "Enginepch.h"

#include "TextComponent.h"

namespace Engine {

	TextComponent::TextComponent(const std::string& text, const sf::Font& font, unsigned int size)
		: m_Text(text, font, size)
	{
	}

	void TextComponent::SetText(const std::string& text)
	{
		m_Text.setString(text);
	}

	void TextComponent::SetPosition(const sf::Vector2f& position)
	{
		m_Text.setPosition(position);
	}

	void TextComponent::SetColor(const sf::Color& color)
	{
		m_Text.setFillColor(color);
	}

	void TextComponent::SetCharacterSize(unsigned int size)
	{
		m_Text.setCharacterSize(size);
	}

}

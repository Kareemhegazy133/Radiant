#include "Enginepch.h"

#include "TextComponent.h"

namespace Engine {

	TextComponent::TextComponent()
		: Text()
	{
	}

	TextComponent::TextComponent(const std::string& text, const sf::Font& font, unsigned int size)
		: Text(text, font, size)
	{
	}

	void TextComponent::SetFont(const sf::Font& font)
	{
		Text.setFont(font);
	}

	void TextComponent::SetText(const std::string& text)
	{
		Text.setString(text);
	}

	void TextComponent::SetPosition(const sf::Vector2f& position)
	{
		Text.setPosition(position);
	}

	void TextComponent::SetColor(const sf::Color& color)
	{
		Text.setFillColor(color);
	}

	void TextComponent::SetCharacterSize(unsigned int size)
	{
		Text.setCharacterSize(size);
	}

}

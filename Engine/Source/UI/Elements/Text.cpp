#include "Enginepch.h"

#include "Text.h"

namespace Engine {

	Text::Text(const std::string& text, const sf::Font& font, unsigned int size, const sf::Color& color)
		: m_Text(text, font, size)
	{
		SetColor(color);
	}

	void Text::SetFont(const sf::Font& font)
	{
		m_Text.setFont(font);
	}

	void Text::SetText(const std::string& text)
	{
		m_Text.setString(text);
	}

	void Text::SetPosition(const sf::Vector2f& position)
	{
		m_Text.setPosition(position);
	}

	void Text::SetColor(const sf::Color& color)
	{
		m_Text.setFillColor(color);
	}

	void Text::SetCharacterSize(unsigned int size)
	{
		m_Text.setCharacterSize(size);
	}

	void Text::SetOrigin(const sf::Vector2f& origin)
	{
		m_Text.setOrigin(origin);
	}

	sf::FloatRect Text::GetLocalBounds()
	{
		return m_Text.getLocalBounds();
	}

	void Text::OnRender(sf::RenderWindow* renderWindow)
	{
		renderWindow->draw(m_Text);
	}

}

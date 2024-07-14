#pragma once

#include <SFML/Graphics.hpp>

#include "UI/UIElement.h"

namespace Engine {

	class Text : public UIElement
	{
	public:
		Text() = default;
		Text(const std::string& text, const sf::Font& font, unsigned int size, const sf::Color& color = sf::Color());

		void SetFont(const sf::Font& font);
		void SetText(const std::string& text);
		void SetPosition(const sf::Vector2f& position);
		void SetColor(const sf::Color& color);
		void SetCharacterSize(unsigned int size);
		void SetOrigin(const sf::Vector2f& origin);

		sf::FloatRect GetLocalBounds();

		inline sf::Drawable& GetDrawable() override { return m_Text; }

		void OnRender(sf::RenderWindow* renderWindow) override;

		operator const sf::Text& () { return m_Text; }
		operator sf::Text& () { return m_Text; }

	private:
		sf::Text m_Text;
	};
}

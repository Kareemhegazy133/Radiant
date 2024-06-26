#pragma once

#include <SFML/Graphics.hpp>
#include "Component.h"

namespace Engine {

	class TextComponent : public Component
	{
	public:

		TextComponent(const std::string& text, const sf::Font& font, unsigned int size);

		void SetText(const std::string& text);
		void SetPosition(const sf::Vector2f& position);
		void SetColor(const sf::Color& color);
		void SetCharacterSize(unsigned int size);

	private:
		sf::Text m_Text;
	};
}
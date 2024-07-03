#pragma once

#include <SFML/Graphics.hpp>

#include "UI/UIElement.h"

namespace Engine {

	class Sprite : public UIElement
	{
	public:
		Sprite() = default;
		Sprite(const sf::Texture& texture, const sf::Vector2f& position = {0.f, 0.f});

		void SetTexture(const sf::Texture& texture);
		void SetPosition(const sf::Vector2f& position);
		void SetScale(const sf::Vector2f& factors);
		void SetScale(float factorX, float factorY);

		inline sf::Sprite& GetDrawable() override { return m_Sprite; }

		operator const sf::Sprite& () { return m_Sprite; }
		operator sf::Sprite& () { return m_Sprite; }

	private:
		sf::Sprite m_Sprite;
	};
}

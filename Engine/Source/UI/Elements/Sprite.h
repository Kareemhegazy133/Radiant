#pragma once

#include <SFML/Graphics.hpp>

#include "UI/UIElement.h"

namespace Engine {

	class Sprite : public UIElement
	{
	public:
		Sprite() = default;
		Sprite(const sf::Texture& texture, const glm::vec2& position = { 0.f, 0.f });

		void SetTexture(const sf::Texture& texture);
		void SetPosition(const glm::vec2& position);
		void SetScale(const glm::vec2& factors);
		void SetScale(float factorX, float factorY);

		inline sf::Drawable& GetDrawable() override { return m_Sprite; }

		void OnRender(sf::RenderWindow* renderWindow) override;

		operator const sf::Sprite& () { return m_Sprite; }
		operator sf::Sprite& () { return m_Sprite; }

	private:
		sf::Sprite m_Sprite;
	};
}

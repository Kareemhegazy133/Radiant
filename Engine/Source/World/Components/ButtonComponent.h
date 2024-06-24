#pragma once

#include <SFML/Graphics.hpp>

#include "Component.h"

namespace Engine {

	class ButtonComponent : public Component
	{
	public:
		ButtonComponent(const sf::Vector2f& position, const sf::Vector2f& size, std::function<void()> onClick);

		void SetSize(const sf::Vector2f& size);
		const sf::Vector2f& GetSize() const;

		void SetPosition(const sf::Vector2f& position);
		void SetPosition(float x, float y);
		const sf::Vector2f& GetPosition() const;

		void SetFillColor(const sf::Color& color);
		void SetOutlineColor(const sf::Color& color);
		void SetOutlineThickness(float thickness);

		bool IsHovered(const sf::Vector2i& mousePos) const;
		void OnClick();

		operator const sf::RectangleShape& () { return m_Rectangle; }
		operator sf::RectangleShape& () { return m_Rectangle; }

	private:
		sf::RectangleShape m_Rectangle;
		std::function<void()> m_OnClick;
	};
}

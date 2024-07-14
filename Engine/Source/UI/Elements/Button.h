#pragma once

#include <SFML/Graphics.hpp>

#include "UI/UIElement.h"
#include "Text.h"

namespace Engine {

	enum class ButtonSize
	{
		Small = 0,
		Medium,
		Large,
		Custom
	};

	class Button : public UIElement
	{
	public:
		Button() = default;
		Button(const sf::Vector2f& position, const sf::Vector2f& size);

		void SetSize(const sf::Vector2f& size);
		const sf::Vector2f& GetSize() const;

		void SetPosition(const sf::Vector2f& position);
		void SetPosition(float x, float y);
		const sf::Vector2f& GetPosition() const;

		void SetButtonCallback(std::function<void()> callback);
		void SetTexture(const sf::Texture* texture);
		void SetHoveredTexture(const sf::Texture* texture);
		void SetText(Text& textElement);
		void SetFillColor(const sf::Color& color);
		void SetOutlineColor(const sf::Color& color);
		void SetOutlineThickness(float thickness);

		inline Text GetText() const { return m_ButtonText; }
		sf::FloatRect GetLocalBounds();

		bool IsHovered(const sf::Vector2i& mousePos) const;
		void OnClick();

		inline sf::Drawable& GetDrawable() override { return m_Rectangle; }

		void OnRender(sf::RenderWindow* renderWindow) override;

		operator const sf::RectangleShape& () { return m_Rectangle; }
		operator sf::RectangleShape& () { return m_Rectangle; }

	private:
		sf::RectangleShape m_Rectangle;
		std::function<void()> m_OnClick;
		bool m_IsHovered = false;
		const sf::Texture* m_NormalTexture = nullptr;
		const sf::Texture* m_HoveredTexture = nullptr;
		ButtonSize m_ButtonSize;
		Text m_ButtonText;
	};
}

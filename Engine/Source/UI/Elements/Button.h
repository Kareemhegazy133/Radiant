#pragma once

#include <SFML/Graphics.hpp>

#include "UI/UIElement.h"
#include "Text.h"

namespace Engine {

	enum class ButtonSize
	{
		Common = 0,
		Small,
		Medium,
		Large,
		Custom
	};

	class Button : public UIElement
	{
	public:
		Button() = default;
		Button(const glm::vec2& position, const glm::vec2& size);

		void SetSize(const glm::vec2& size);
		const glm::vec2& GetSize() const;

		void SetPosition(const glm::vec2& position);
		void SetPosition(float x, float y);
		const glm::vec2& GetPosition() const;

		void SetDisabled(bool disable);

		void SetButtonCallback(std::function<void()> callback);
		void SetTexture(const sf::Texture* texture);
		void SetNormalTexture(const sf::Texture* texture);
		void SetHoveredTexture(const sf::Texture* texture);
		void SetDisabledTexture(const sf::Texture* texture);
		void SetText(Text& textElement);
		void SetFillColor(const sf::Color& color);
		void SetOutlineColor(const sf::Color& color);
		void SetOutlineThickness(float thickness);

		inline Text GetText() const { return m_ButtonText; }
		sf::FloatRect GetLocalBounds();

		bool IsHovered(const sf::Vector2i& mousePos) const;
		bool IsDisabled() const;
		void OnClick();

		inline sf::Drawable& GetDrawable() override { return m_Rectangle; }

		void OnRender(sf::RenderWindow* renderWindow) override;

		operator const sf::RectangleShape& () { return m_Rectangle; }
		operator sf::RectangleShape& () { return m_Rectangle; }

	private:
		sf::RectangleShape m_Rectangle;
		std::function<void()> m_OnClick;
		bool m_IsHovered = false;
		bool m_IsDisabled = false;
		const sf::Texture* m_NormalTexture = nullptr;
		const sf::Texture* m_HoveredTexture = nullptr;
		const sf::Texture* m_DisabledTexture = nullptr;
		ButtonSize m_ButtonSize;
		Text m_ButtonText;
	};
}

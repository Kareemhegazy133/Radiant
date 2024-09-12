#include "Enginepch.h"
#include "Core/Input.h"

#include "Core/GameApplication.h"

namespace Engine {

	bool Input::IsKeyPressed(KeyCode key)
	{
		auto window = GameApplication::GetRenderWindow();
		return window->hasFocus() && sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(key));
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		auto window = GameApplication::GetRenderWindow();
		return window->hasFocus() && sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(button));
	}

	// TODO: Use glm here when glfw implemented
	std::pair<float, float> Input::GetMousePosition()
	{
		auto window = GameApplication::GetRenderWindow();
		sf::Vector2i pos = sf::Mouse::getPosition(*window);

		return { static_cast<float>(pos.x), static_cast<float>(pos.y) };
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return y;
	}

}
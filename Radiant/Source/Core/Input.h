#pragma once

#include <glm/glm.hpp>

#include "Core/KeyCodes.h"
#include "Core/MouseCodes.h"

namespace Radiant {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
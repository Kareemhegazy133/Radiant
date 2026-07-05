#pragma once

#include <glm/glm.hpp>

#include "Radiant/Core/KeyCodes.h"
#include "Radiant/Core/MouseCodes.h"

namespace Radiant {

	/**
	 * Static polling input API: each call queries the current device state from
	 * the platform layer directly, independent of the event system (the Handled
	 * flag does not apply here). Requires the application window to exist — it
	 * reaches through GameApplication::Get() — and is main-thread only. Mouse
	 * position is in window client-area coordinates, origin top-left.
	 */
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
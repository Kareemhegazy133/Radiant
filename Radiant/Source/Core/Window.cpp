#include "rdpch.h"
#include "Core/Window.h"

#ifdef RADIANT_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Radiant
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
#ifdef RADIANT_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
#else
		RADIANT_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}
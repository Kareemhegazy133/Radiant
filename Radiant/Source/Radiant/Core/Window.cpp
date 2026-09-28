#include "Radiant/rdpch.h"
#include "Radiant/Core/Window.h"

#ifdef RADIANT_PLATFORM_WINDOWS
#include "Radiant/Platform/Windows/WindowsWindow.h"
#endif

namespace Radiant
{
	Scope<Window> Window::Create(const WindowSpecification& specification)
	{
#ifdef RADIANT_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(specification);
#else
		RADIANT_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}
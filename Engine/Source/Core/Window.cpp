#include "Enginepch.h"
#include "Core/Window.h"

#ifdef ENGINE_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Engine
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
		#ifdef ENGINE_PLATFORM_WINDOWS
			return CreateScope<WindowsWindow>(props);
		#else
			ENGINE_ASSERT(false, "Unknown platform!");
			return nullptr;
		#endif
	}

}
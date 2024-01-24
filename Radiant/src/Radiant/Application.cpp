#include "Application.h"

#include "Radiant/Events/ApplicationEvent.h"
#include "Radiant/Log.h"

namespace Radiant {

	Application::Application()
	{

	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication))
		{
			RD_TRACE(e);
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			RD_TRACE(e);
		}

		while (true);
	}
}
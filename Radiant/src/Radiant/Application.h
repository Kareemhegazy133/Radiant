#pragma once

#include "Core.h"

namespace Radiant {

	class RADIANT_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// To be define in CLIENT
	Application* CreateApplication();
}
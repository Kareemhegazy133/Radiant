#pragma once

#include "Core/Base.h"
#include "Core/GameApplication.h"

#ifdef RADIANT_PLATFORM_WINDOWS

extern Radiant::GameApplication* Radiant::CreateGameApplication(GameApplicationCommandLineArgs);

int main(int argc, char** argv)
{
	Radiant::Log::Init();

	RADIANT_PROFILE_BEGIN_SESSION("Startup", "RadiantProfile-Startup.json");
	auto gameApp = Radiant::CreateGameApplication({ argc, argv });
	RADIANT_PROFILE_END_SESSION();

	RADIANT_PROFILE_BEGIN_SESSION("Runtime", "RadiantProfile-Runtime.json");
	gameApp->Run();
	RADIANT_PROFILE_END_SESSION();

	RADIANT_PROFILE_BEGIN_SESSION("Shutdown", "RadiantProfile-Shutdown.json");
	delete gameApp;
	RADIANT_PROFILE_END_SESSION();
}

#endif
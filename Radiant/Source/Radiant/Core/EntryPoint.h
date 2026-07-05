#pragma once

#include "Radiant/Core/Base.h"
#include "Radiant/Core/GameApplication.h"

#ifdef RADIANT_PLATFORM_WINDOWS

extern Radiant::GameApplication* Radiant::CreateGameApplication();

int main(int argc, char** argv)
{
	Radiant::Log::Init();

	RADIANT_PROFILE_BEGIN_SESSION("Startup", "RadiantProfile-Startup.json");
	auto gameApp = Radiant::CreateGameApplication();
	RADIANT_PROFILE_END_SESSION();

	RADIANT_PROFILE_BEGIN_SESSION("Runtime", "RadiantProfile-Runtime.json");
	gameApp->Run();
	RADIANT_PROFILE_END_SESSION();

	RADIANT_PROFILE_BEGIN_SESSION("Shutdown", "RadiantProfile-Shutdown.json");
	delete gameApp;
	RADIANT_PROFILE_END_SESSION();
}

#ifdef RD_DIST
// Dist builds link as WindowedApp (no console window), whose entry point is
// WinMain. Forward to main() so there is exactly one real entry path.
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

#endif
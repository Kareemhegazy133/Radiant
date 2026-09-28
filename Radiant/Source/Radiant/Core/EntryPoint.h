#pragma once

#include "Radiant/Core/Base.h"
#include "Radiant/Core/GameApplication.h"

#ifdef RADIANT_PLATFORM_WINDOWS

extern Radiant::GameApplication* Radiant::CreateGameApplication();

// The engine owns the entry point so init/shutdown ordering is guaranteed:
// logging first (or nothing else can report its failure), then the game's
// application, which main() owns and deletes. GameApplication::Run() is
// private and befriends main — the loop cannot be started from game code.
int main(int argc, char** argv)
{
	Radiant::Log::Init();

	RADIANT_PROFILE_BEGIN_SESSION("Startup", "RadiantProfile-Startup.json");
	auto gameApp = Radiant::CreateGameApplication();
	RADIANT_ASSERT(gameApp, "CreateGameApplication() returned null");
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
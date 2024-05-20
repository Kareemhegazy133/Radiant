#pragma once

#include "Core/Base.h"
#include "Core/GameApplication.h"

#ifdef ENGINE_PLATFORM_WINDOWS

extern Engine::GameApplication* Engine::CreateGameApplication(GameApplicationCommandLineArgs);

int main(int argc, char** argv)
{
	Engine::Log::Init();
	ENGINE_INFO("Initialized Log!");

	auto gameApp = Engine::CreateGameApplication({ argc, argv });
	gameApp->Run();
	delete gameApp;
}

#endif
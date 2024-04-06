#pragma once

// For use by Game applications

#include "Core/GameApplication.h"
#include "Core/Log.h"


extern Engine::GameApplication* Engine::CreateGameApplication();

int main(int argc, char** argv)
{
	Engine::Log::Init();
	ENGINE_INFO("Initialized Log!");

	auto gameApp = Engine::CreateGameApplication();
	gameApp->Run();
	delete gameApp;
}

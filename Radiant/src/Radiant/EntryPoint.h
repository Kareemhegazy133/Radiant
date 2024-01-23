#pragma once

#ifdef RD_PLATFORM_WINDOWS

extern Radiant::Application* Radiant::CreateApplication();

int main(int argc, char** argv)
{
	Radiant::Log::Init();
	RD_CORE_WARN("Initialized Log!");
	int a = 5;
	RD_INFO("Hello! Var={0}", a);

	auto app = Radiant::CreateApplication();
	app->Run();
	delete app;
}

#endif
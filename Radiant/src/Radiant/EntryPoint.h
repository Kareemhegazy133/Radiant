#pragma once

#ifdef RD_PLATFORM_WINDOWS

extern Radiant::Application* Radiant::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Radiant::CreateApplication();
	app->Run();
	delete app;
}

#endif
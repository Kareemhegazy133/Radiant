#include "Engine.h"
#include "Core/EntryPoint.h"
#include "GameConfig.h"


class ExampleLayer : public Engine::Layer
{
public:
	ExampleLayer() : Layer("Example")
	{

	}

	void OnUpdate(Engine::Timestep ts) override
	{
		//GAME_TRACE("Delta time: {0}s ({1})ms", ts.GetSeconds(), ts.GetMilliseconds());
	}

	void OnEvent(Engine::Event& event) override
	{
		//GAME_TRACE(event.ToString());
	}

};

class TheReaper : public  Engine::GameApplication
{

public:
	TheReaper() : GameApplication()
	{
		PushLayer(new ExampleLayer());
	}

};

Engine::GameApplication* Engine::CreateGameApplication()
{
	return new TheReaper();
}


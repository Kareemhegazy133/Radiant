#include <Radiant.h>

class ExampleLayer : public Radiant::Layer
{
public:
	ExampleLayer(): Layer("Example")
	{

	}

	void OnUpdate() override
	{
		RD_INFO("ExampleLayer::Update");
	}

	void OnEvent(Radiant::Event& event) override
	{
		RD_TRACE("{0}", event);
	}

};

class Sandbox : public Radiant::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}
};

Radiant::Application* Radiant::CreateApplication()
{
	return new Sandbox();
}
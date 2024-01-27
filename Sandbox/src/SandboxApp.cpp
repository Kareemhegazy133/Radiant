#include <Radiant.h>

class ExampleLayer : public Radiant::Layer
{
public:
	ExampleLayer(): Layer("Example")
	{

	}

	void OnUpdate() override
	{

		if (Radiant::Input::IsKeyPressed(RD_KEY_TAB))
		{
			RD_TRACE("Tab Key is pressed (poll)!");
		}
	}

	void OnEvent(Radiant::Event& event) override
	{
		if (event.GetEventType() == Radiant::EventType::KeyPressed)
		{
			Radiant::KeyPressedEvent& e = (Radiant::KeyPressedEvent&)event;
			if (e.GetKeyCode() == RD_KEY_TAB)
				RD_TRACE("Tab key is pressed (event)!");
			RD_TRACE("{0}", (char)e.GetKeyCode());
		}
	}

};

class Sandbox : public Radiant::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushOverlay(new Radiant::ImGuiLayer());
	}

	~Sandbox()
	{

	}
};

Radiant::Application* Radiant::CreateApplication()
{
	return new Sandbox();
}
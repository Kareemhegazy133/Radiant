#include <Radiant.h>

class Sandbox : public Radiant::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}
};

Radiant::Application* Radiant::CreateApplication()
{
	return new Sandbox();
}
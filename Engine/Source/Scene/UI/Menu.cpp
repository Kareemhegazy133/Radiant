#include "Enginepch.h"

#include "Menu.h"

namespace Engine {

	Menu::Menu(const std::string& menuName)
		: UIElement(Scene::GetScene().CreateGameObject(menuName), &Scene::GetScene())
	{

	}

	Menu::~Menu()
	{
	}

}
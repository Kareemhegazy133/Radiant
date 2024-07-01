#pragma once

#include "UIElement.h"

namespace Engine {

	class Menu : public UIElement
	{
	public:
		Menu(const std::string& menuName);
		virtual ~Menu();

		virtual void Initialize() = 0;

		virtual void OnUpdate(Timestep ts) = 0;

		virtual void Show() = 0;
		virtual void Hide() = 0;
		virtual bool IsVisible() const = 0;
		
	protected:
		sf::RenderWindow* m_RenderWindow = static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow());
		MetadataComponent& metadata = GetComponent<MetadataComponent>();

	};
}

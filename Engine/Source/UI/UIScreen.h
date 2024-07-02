#pragma once

#include "Core/GameApplication.h"
#include "Core/Timestep.h"

#include "UIElement.h"

namespace Engine {

	class UIScreen
	{
	public:
		virtual ~UIScreen()
		{
			for (auto element : m_Elements)
			{
				delete element;
			}
			m_Elements.clear();
		}

		virtual void Initialize() = 0;
		
		template<typename T, typename... Args>
		T& AddElement(Args&&... args)
		{
			m_Elements.emplace_back(new T(std::forward<Args>(args)...));
			return static_cast<T&>(*m_Elements.back());
		}

		virtual void OnUpdate(Timestep ts)
		{
			for (UIElement* element : m_Elements)
			{
				ENGINE_ASSERT(element, "Element must not be nullptr!");
				m_RenderWindow->draw(element->GetDrawable());
			}
		}

		inline virtual void SetVisibility(bool visible) { m_IsVisible = visible; }
		inline virtual bool IsVisible() const { return m_IsVisible; }
		
	protected:
		sf::RenderWindow* m_RenderWindow = static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow());

		bool m_IsVisible = false;

		std::vector<UIElement*> m_Elements;
	};
}

#pragma once

namespace Engine {

    class UIElement
    {
    public:
        virtual ~UIElement() = default;

        inline virtual sf::Drawable& GetDrawable() = 0;

        virtual void OnRender(sf::RenderWindow* renderWindow) = 0;

        //virtual void Serialize() = 0;
        //virtual void Deserialize() = 0;
    };
}

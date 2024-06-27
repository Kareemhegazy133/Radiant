#pragma once

#include "Core/GameApplication.h"

#include "Scene.h"

namespace Engine {

    class Level : public Scene
    {
    public:
        Level()
            : m_RenderWindow(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow())) {}
        virtual ~Level() = default;

        virtual void OnUpdate(Timestep ts) = 0;
        virtual void OnRender() = 0;

        virtual void OnEvent(Event& e) = 0;

    protected:
        sf::RenderWindow* m_RenderWindow;
    };
}
#pragma once

#include "Events/Event.h"

#include "Scene.h"

namespace Engine {

    class Level : public Scene
    {

    public:
        virtual ~Level() = default;

        virtual void OnUpdate(Timestep ts) = 0;
        virtual void OnRender() = 0;

        virtual void OnEvent(Event& e) = 0;

    };
}
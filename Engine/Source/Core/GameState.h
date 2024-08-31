#pragma once

#include "Core/Timestep.h"
#include "Events/Event.h"

namespace Engine {

    class GameState
    {
    public:
        virtual ~GameState() = default;

        // Called when the state is entered
        virtual void OnEnter() = 0;

        // Called when the state is exited
        virtual void OnExit() = 0;

        // Called every frame to update the state
        virtual void OnUpdate(Timestep ts) = 0;

        // Called when an event occurs
        virtual void OnEvent(Event& e) = 0;
    };
}

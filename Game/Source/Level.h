#pragma once

#include "Engine.h"

namespace Engine {

    class Level : public World {
    public:
        Level(sf::RenderWindow* renderWindow);
        ~Level();

        void OnUpdate(Timestep ts) override;
        void OnRender();
    };

}

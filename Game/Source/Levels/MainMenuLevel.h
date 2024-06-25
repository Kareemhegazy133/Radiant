#pragma once

#include <Engine.h>

using namespace Engine;

class MainMenuLevel : public Level
{

public:
    MainMenuLevel();
    ~MainMenuLevel();

    void OnUpdate(Timestep ts) override;
    void OnRender() override;

    void OnEvent(Event& e);

};


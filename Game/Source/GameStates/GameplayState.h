#pragma once

#include <Engine.h>

#include "GameContext.h"

#include "Gameplay/Entities/Characters/Player.h"

using namespace Engine;

class GameLayer;

class GameplayState : public GameState
{
public:
    GameplayState(GameLayer* gameLayer)
        : m_GameLayer(gameLayer)
    {
    }

    void OnEnter() override
    {
        m_Level = CreateScope<Level>();
        GameContext::SetLevel(m_Level.get());

        Entity player = m_Level->CreateEntity("Player");
        player.AddComponent<NativeScriptComponent>().Bind<Player>();
    }

    void OnExit() override;

    void OnUpdate(Timestep ts) override
    {
        m_Level->OnUpdate(ts);
        m_Level->OnRender();
    }

    void OnEvent(Event& e) override
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(GameplayState::OnKeyPressed));
    }

    bool OnKeyPressed(KeyPressedEvent& e);

private:
    Scope<Level> m_Level;
    GameLayer* m_GameLayer = nullptr;

};

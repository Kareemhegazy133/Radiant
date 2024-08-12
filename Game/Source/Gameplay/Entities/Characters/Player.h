#pragma once

#include <Engine.h>

#include "Gameplay/Entities/EntityStates.h"

using namespace Engine;

class Player : public Character
{

public:
    Player();
    ~Player();
    
    void SetupAnimations();
    void OnUpdate(Timestep ts) override;

    void SetState(CharacterState newState);
    void OnEnterState(CharacterState state);
    void OnExitState(CharacterState state);

    inline AttributesComponent* GetAttributesComponent() const { return &attributes; }
    inline CharacterComponent* GetCharacterComponent() const { return &character; }

    void OnCollisionBegin(Entity& other) override;
    void OnCollisionEnd(Entity& other) override;

private:
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);

    CharacterState m_CurrentState;
};
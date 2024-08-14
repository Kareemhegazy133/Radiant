#pragma once

#include <Engine.h>

using namespace Engine;

enum class PlayerState
{
    Idle = 0,
    Walking,
    Running,
    Jumping,
    Throwing,
    Attacking
};

class Player : public Character
{
    
public:
    Player();
    ~Player();
    
    void OnUpdate(Timestep ts) override;

    inline AttributesComponent* GetAttributesComponent() const { return &attributes; }
    inline CharacterComponent* GetCharacterComponent() const { return &character; }

    void OnCollisionBegin(Entity& other) override;
    void OnCollisionEnd(Entity& other) override;

private:
    void SetupAnimations();
    void SetupStateMachine();

private:
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    StateMachine m_StateMachine;
};
#pragma once

#include "Character.h"

#include "Gameplay/GameAttributeSet.h"
#include "Menus/CharacterInfoMenu.h"

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
    void OnCreate() override;
    void OnUpdate(Timestep ts) override;
    void OnDestroy() override;

    GameAttributeSet* GetAttributeSet() const { return m_AttributeSet; };

    void OnCollisionBegin(Entity& other) override;
    void OnCollisionEnd(Entity& other) override;

private:
    void SetupAnimations();
    void SetupStateMachine();

private:
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    StateMachine m_StateMachine;

    CharacterInfoMenu* m_CharacterInfoMenu = nullptr;
    GameAttributeSet* m_AttributeSet = nullptr;
};
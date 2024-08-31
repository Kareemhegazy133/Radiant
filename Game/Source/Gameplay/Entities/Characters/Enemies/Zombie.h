#pragma once

#include "Gameplay/Entities/Characters/Character.h"
#include "Gameplay/GameAttributeSet.h"

using namespace Engine;

enum class EnemyState
{
    Idle = 0,
    Walking,
    Running,
    Jumping,
    Throwing,
    Attacking
};

class Zombie : public Character
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

private:
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    GameAttributeSet* m_AttributeSet = nullptr;
};
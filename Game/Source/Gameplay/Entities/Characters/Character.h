#pragma once

#include <Engine.h>

using namespace Engine;

class Character : public ScriptableEntity
{
public:

    virtual void OnCreate()
    {
        GetComponent<MetadataComponent>().Type = typeid(Character);
        AddComponent<SpriteComponent>();
        AddComponent<AnimationComponent>();
        AddComponent<AbilitySystemComponent>();
    }

    virtual void OnUpdate(Timestep ts) = 0;
    virtual void OnDestroy() = 0;

    virtual void OnCollisionBegin(Entity& other) = 0;
    virtual void OnCollisionEnd(Entity& other) = 0;

public:
    int Level;
    int Coins;
    int Diamonds;

    float CurrentHealth = 0.f;
    float CurrentStamina = 0.f;
    float Speed;

    sf::Vector2f Direction;

};

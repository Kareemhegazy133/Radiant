#pragma once

#include <Engine.h>

using namespace Engine;

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
    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
};
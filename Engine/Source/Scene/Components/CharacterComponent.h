#pragma once

#include "Scene/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class CharacterComponent : public Component
    {
    public:

        CharacterComponent() = default;
        CharacterComponent(const CharacterComponent&) = default;

    public:
        int Level;
        int Coins;
        int Diamonds;

        float CurrentHealth = 0.f;
        float CurrentStamina = 0.f;
        float Speed;

    };
}

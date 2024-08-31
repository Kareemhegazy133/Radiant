#pragma once

#include "AttributeSet.h"

namespace Engine {

    class GameplayEffect
    {
    public:
        virtual ~GameplayEffect() = default;

        virtual void ApplyEffect(AttributeSet& target) = 0;

    protected:
        float Duration = 0.0f;
        float Magnitude = 0.0f;
    };

}

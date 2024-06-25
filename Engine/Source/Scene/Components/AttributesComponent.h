#pragma once

#include "Scene/Components/Component.h"

enum class Attributes;

namespace Engine {

    class AttributesComponent : public Component
    {
    public:

        AttributesComponent();

        void SetAttribute(Attributes attribute, float value);
        float GetAttribute(Attributes attribute) const;

    private:
        void EnsureCapacity(size_t capacity);

    private:
        // Pair of float (value) and bool (whether set or not)
        std::vector<std::pair<float, bool>> m_Values;

    };

}

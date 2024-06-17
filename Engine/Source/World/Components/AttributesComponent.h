#pragma once

#include "World/Components/Component.h"

enum class Attributes;

namespace Engine {

    class AttributesComponent : public Component
    {
    public:
        AttributesComponent()
            : m_Values(10, std::make_pair(0.0f, false)) // Initialize with a default of 10 elements, all set to 0.0f and false
        {
        }

        // Method to set an attribute value
        void SetAttribute(Attributes attribute, float value)
        {
            int id = static_cast<int>(attribute);
            EnsureCapacity(id + 1); // Ensure capacity for at least id + 1 elements
            m_Values[id] = std::make_pair(value, true);
        }

        // Method to get an attribute value
        float GetAttribute(Attributes attribute) const
        {
            int id = static_cast<int>(attribute);
            if (id >= 0 && id < m_Values.size())
            {
                // Warn if the attribute has not been set
                if (!m_Values[id].second) 
                {
                    ENGINE_WARN("Attribute {0} not set, returning default Attribute value: {1}", id, m_Values[id].first);
                }
                return m_Values[id].first;
            }
            ENGINE_ERROR("Attribute ID out of range");
            throw std::out_of_range("Attribute ID out of range");
        }

    private:
        void EnsureCapacity(size_t capacity)
        {
            if (capacity > m_Values.size())
            {
                m_Values.resize(capacity, std::make_pair(0.0f, false));
            }
        }

    private:
        // Pair of float (value) and bool (whether set or not)
        std::vector<std::pair<float, bool>> m_Values;

    };

}

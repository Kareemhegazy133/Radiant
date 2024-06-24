#include "Enginepch.h"

#include "AttributesComponent.h"

namespace Engine {

    AttributesComponent::AttributesComponent()
        : m_Values(10, std::make_pair(0.0f, false)) // Initialize with a default of 10 elements, all set to 0.0f and false
    {
    }

    void AttributesComponent::SetAttribute(Attributes attribute, float value)
    {
        int id = static_cast<int>(attribute);
        EnsureCapacity(id + 1); // Ensure capacity for at least id + 1 elements
        m_Values[id] = std::make_pair(value, true);
    }

    float AttributesComponent::GetAttribute(Attributes attribute) const
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

    void AttributesComponent::EnsureCapacity(size_t capacity)
    {
        if (capacity > m_Values.size()) {
            m_Values.resize(capacity, std::make_pair(0.0f, false));
        }
    }

}

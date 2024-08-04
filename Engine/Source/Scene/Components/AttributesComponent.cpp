#include "Enginepch.h"

#include "AttributesComponent.h"

namespace Engine {

    AttributesComponent::AttributesComponent()
        : m_Values(10, std::make_pair(0.0f, false)), // Initialize with a default of 10 elements, all set to 0.0f and false
        m_AttributePointsSpent(10, 0)
    {
    }

    void AttributesComponent::SetAttribute(Attributes attribute, float value, int points)
    {
        int id = static_cast<int>(attribute);
        EnsureCapacity(id + 1); // Ensure capacity for at least id + 1 elements
        m_Values[id] = std::make_pair(value, true);
        m_AttributePointsSpent[id] = points;
        m_TotalPointsSpent += points;
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

    void AttributesComponent::UpgradeAttribute(Attributes attribute, int points)
    {
        if (m_AttributePointsAvailable <= 0)
        {
            GAME_INFO("Not Enough Attribute Points available to upgrade attribute");
            return;
        }

        int id = static_cast<int>(attribute);

        m_Values[id].first = CalculateUpgradeValue(attribute, points);
        m_Values[id].second = true;
        m_AttributePointsAvailable -= points;
        m_AttributePointsSpent[id] += points;
        m_TotalPointsSpent += points;
    }

    float AttributesComponent::CalculateUpgradeValue(Attributes attribute, int levels) const
    {
        int id = static_cast<int>(attribute);

        float currentValue = m_Values[id].first;
        int currentLevel = m_AttributePointsSpent[id];

        if (levels == 0)
        {
            return currentValue + ((currentLevel + 1) * m_GrowthRate);
        }

        float result = 0.0f;
        for (int i = 0; i < levels; i++)
        {
            result += currentValue + ((currentLevel + 1) * m_GrowthRate);
            currentValue = result;
            currentLevel++;
        }

        return result;
    }

    void AttributesComponent::AddAttributePoints(int amount)
    {
        m_AttributePointsAvailable += amount;
    }

    int AttributesComponent::GetAttributePointsSpent(Attributes attribute) const
    {
        int id = static_cast<int>(attribute);
        if (id >= 0 && id < m_AttributePointsSpent.size())
        {
            return m_AttributePointsSpent[id];
        }
        ENGINE_ERROR("Attribute ID out of range");
        throw std::out_of_range("Attribute ID out of range");
    }

    void AttributesComponent::EnsureCapacity(size_t capacity)
    {
        if (capacity > m_Values.size()) {
            m_Values.resize(capacity, std::make_pair(0.0f, false));
            m_AttributePointsSpent.resize(capacity, 0);
        }
    }

}

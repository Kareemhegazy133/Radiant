#pragma once

#include "Scene/Components/Component.h"

enum class Attributes;

namespace Engine {

    class AttributesComponent : public Component
    {
    public:

        AttributesComponent();

        void SetAttribute(Attributes attribute, float value, int points);
        float GetAttribute(Attributes attribute) const;
        
        void UpgradeAttribute(Attributes attribute, int points = 1);
        //float CalculateUpgradeValue(Attributes attribute) const;
        float CalculateUpgradeValue(Attributes attribute, int levels = 0) const;

        void AddAttributePoints(int amount);
        inline int GetAttributePointsAvailable() const { return m_AttributePointsAvailable; }

        int GetAttributePointsSpent(Attributes attribute) const;
        inline int GetTotalAttributePointsSpent() const { return m_TotalPointsSpent; }

    private:
        void EnsureCapacity(size_t capacity);

    private:
        // Growth rate of 10% for upgrading attributes
        float m_GrowthRate = 0.35f;
        // Pair of float (value of attribute) and bool (whether set or not)
        std::vector<std::pair<float, bool>> m_Values;

        int m_AttributePointsAvailable = 0;

        // Track points spent on each attribute
        std::vector<int> m_AttributePointsSpent;
        int m_TotalPointsSpent = 0;
    };

}

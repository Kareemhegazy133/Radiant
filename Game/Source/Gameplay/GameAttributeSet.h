#pragma once

#include <Engine.h>

using namespace Engine;

enum class Attributes
{
    Health = 0,
    Stamina,
    Strength,
    Defense,
    Magic
};

class GameAttributeSet : AttributeSet
{
public:

    void SetAttribute(Attributes attribute, float value, int points);
    float GetAttribute(Attributes attribute) const;

    void UpgradeAttribute(Attributes attribute, int points = 1);
    float CalculateUpgradeValue(Attributes attribute, int levels = 0) const;

    void AddAttributePoints(int amount);
    int GetAttributePointsAvailable() const { return m_AttributePointsAvailable; }

    int GetAttributePointsSpent(Attributes attribute) const;
    int GetTotalAttributePointsSpent() const { return m_TotalPointsSpent; }

    int GetTotalAttributesCount() const;

private:
    // Override methods from base class
    float GetAttribute(const std::string& attributeName) const override;
    void SetAttribute(const std::string& attributeName, float value, int points) override;

    // Helper functions
    std::string AttributeToString(Attributes attribute) const;

private:
    float m_GrowthRate = 0.35f;
    int m_AttributePointsAvailable = 0;
    int m_TotalPointsSpent = 0;

    std::unordered_map<std::string, std::pair<float, int>> m_AttributesMap;
};

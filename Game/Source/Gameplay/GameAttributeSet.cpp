#include "GameAttributeSet.h"

void GameAttributeSet::SetAttribute(Attributes attribute, float value, int points)
{
    std::string attributeName = AttributeToString(attribute);
    SetAttribute(attributeName, value, points);
}

float GameAttributeSet::GetAttribute(Attributes attribute) const
{
    std::string attributeName = AttributeToString(attribute);
    return GetAttribute(attributeName);
}

void GameAttributeSet::UpgradeAttribute(Attributes attribute, int points)
{
    if (m_AttributePointsAvailable <= 0)
    {
        std::cerr << "Not Enough Attribute Points available to upgrade attribute" << std::endl;
        return;
    }

    std::string attributeName = AttributeToString(attribute);
    auto& attrPair = m_AttributesMap[attributeName];
    attrPair.first = CalculateUpgradeValue(attribute, points);
    m_AttributePointsAvailable -= points;
    attrPair.second += points;
    m_TotalPointsSpent += points;
}

float GameAttributeSet::CalculateUpgradeValue(Attributes attribute, int levels) const {
    std::string attributeName = AttributeToString(attribute);
    auto it = m_AttributesMap.find(attributeName);
    if (it == m_AttributesMap.end())
    {
        throw std::out_of_range("Attribute not found");
    }

    float currentValue = it->second.first;
    int currentLevel = it->second.second;

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

void GameAttributeSet::AddAttributePoints(int amount)
{
    m_AttributePointsAvailable += amount;
}

int GameAttributeSet::GetAttributePointsSpent(Attributes attribute) const
{
    std::string attributeName = AttributeToString(attribute);
    auto it = m_AttributesMap.find(attributeName);
    if (it != m_AttributesMap.end())
    {
        return it->second.second;
    }
    throw std::out_of_range("Attribute not found");
}

int GameAttributeSet::GetTotalAttributesCount() const
{
    return m_AttributesMap.size();
}

float GameAttributeSet::GetAttribute(const std::string& attributeName) const
{
    auto it = m_AttributesMap.find(attributeName);
    if (it != m_AttributesMap.end())
    {
        return it->second.first;
    }
    throw std::out_of_range("Attribute not found");
}

void GameAttributeSet::SetAttribute(const std::string& attributeName, float value, int points)
{
    m_AttributesMap[attributeName] = std::make_pair(value, points);
    m_TotalPointsSpent += points;
}

std::string GameAttributeSet::AttributeToString(Attributes attribute) const
{
    switch (attribute)
    {
    case Attributes::Health: return "Health";
    case Attributes::Stamina: return "Stamina";
    case Attributes::Strength: return "Strength";
    case Attributes::Defense: return "Defense";
    case Attributes::Magic: return "Magic";
    default: throw std::invalid_argument("Unknown attribute");
    }
}

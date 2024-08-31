#pragma once

namespace Engine {

    class AttributeSet
    {
    public:
        virtual ~AttributeSet() = default;

        // Virtual methods that games can override
        virtual float GetAttribute(const std::string& attributeName) const = 0;
        virtual void SetAttribute(const std::string& attributeName, float value, int points) = 0;

    };

}

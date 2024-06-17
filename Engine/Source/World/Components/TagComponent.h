#pragma once

#include "World/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class TagComponent : public Component
    {
    public:
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}

        operator const std::string& () { return Tag; }
    };
}


#pragma once

#include "World/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class MetadataComponent : public Component
    {
    public:

        MetadataComponent() = default;
        MetadataComponent(const MetadataComponent&) = default;
        MetadataComponent(const std::string& tag, bool isActive = true)
            : Tag(tag), IsActive(isActive) {}

    public:
        std::string Tag;
        bool IsActive = true;

        std::function<void(Timestep&)> OnUpdate = nullptr;
    };
}


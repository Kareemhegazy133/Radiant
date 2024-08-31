#pragma once

#include "Enginepch.h"

#include "Core/UUID.h"

namespace Engine {

    class MetadataComponent
    {
    public:

        MetadataComponent() = default;
        MetadataComponent(const MetadataComponent&) = default;
        MetadataComponent(const std::string& tag, bool isActive = true)
            : Tag(tag), IsActive(isActive) {}

    public:
        UUID ID;
        std::string Tag;
        bool IsActive = true;

    };
}


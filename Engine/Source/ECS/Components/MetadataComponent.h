#pragma once

#include "Enginepch.h"

#include <typeindex>

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

        // Initialize with 'void' as a default type
        std::type_index Type = typeid(void);
    };
}


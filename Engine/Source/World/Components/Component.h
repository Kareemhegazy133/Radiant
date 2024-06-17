#pragma once

namespace Engine {

    class Component
    {
    public:
        virtual ~Component() = default;

        //virtual void Serialize() = 0;
        //virtual void Deserialize() = 0;
    };
}
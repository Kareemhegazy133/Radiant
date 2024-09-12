#pragma once

#include "Enginepch.h"

namespace Engine {

    class ScriptableEntity;

    class NativeScriptComponent
    {
    public:

        template<typename T>
        void Bind()
        {
            InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
        }

        template<typename T>
        void Bind(ScriptableEntity* abilityOwner)
        {
            InstantiateScript = [abilityOwner]() { return static_cast<ScriptableEntity*>(new T(abilityOwner)); };
            DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
        }

    public:
        ScriptableEntity* Instance = nullptr;

        std::function<ScriptableEntity* ()> InstantiateScript;
        std::function<void(NativeScriptComponent*)> DestroyScript;
    };
}


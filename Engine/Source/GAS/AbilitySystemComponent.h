#pragma once

#include "Ability.h"

namespace Engine {

    class AbilitySystemComponent
    {
    public:

        template<typename T>
        void AddAbility(const std::string& abilityName, ScriptableEntity* owner, Level* level)
        {
            ENGINE_ASSERT(std::is_base_of<Ability, T>::value, "T must inherit from Ability");
            Entity ability = level->CreateEntity(abilityName);
            ability.AddComponent<NativeScriptComponent>().Bind<T>(owner);

            m_Abilities.emplace_back(ability);
        }

        bool ActivateAbility(size_t index);

        const std::vector<Entity>& GetAbilities() const;

    private:
        std::vector<Entity> m_Abilities;
    };

}
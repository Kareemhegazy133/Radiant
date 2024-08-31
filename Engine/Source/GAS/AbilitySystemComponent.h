#pragma once

#include "Ability.h"

namespace Engine {

    class AbilitySystemComponent
    {
    public:

        template<typename T, typename... Args>
        void AddAbility(Args&&... args)
        {
            ENGINE_ASSERT(std::is_base_of<Ability, T>::value, "T must inherit from Ability");
            m_Abilities.emplace_back(CreateRef<T>(std::forward<Args>(args)...));
        }

        bool ActivateAbility(size_t index);

        const std::vector<Ref<Ability>>& GetAbilities() const;

    private:
        std::vector<Ref<Ability>> m_Abilities;
    };

}
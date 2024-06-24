#pragma once

#include "World/Components/Component.h"

namespace Engine {

    class GameplayEntity;
    class Ability;

    class AbilitiesComponent : public Component
    {
    public:
        AbilitiesComponent();

        template<typename T, typename... Args>
        void AddAbility(Args&&... args)
        {
            ENGINE_ASSERT(std::is_base_of<Ability, T>::value, "T must inherit from Ability");
            m_Abilities.emplace_back(CreateRef<T>(std::forward<Args>(args)...));
        }

        void ActivateAbility(size_t index, GameplayEntity& caster);

        const std::vector<Ref<Ability>>& GetAbilities() const;

    private:
        std::vector<Ref<Ability>> m_Abilities;
    };

}

#pragma once

#include "Scene/Components/Component.h"

namespace Engine {

    class Entity;
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

        void ActivateAbility(size_t index, Entity& caster);

        const std::vector<Ref<Ability>>& GetAbilities() const;

    private:
        std::vector<Ref<Ability>> m_Abilities;
    };

}

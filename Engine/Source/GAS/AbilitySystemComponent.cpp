#include "Enginepch.h"

#include "AbilitySystemComponent.h"

namespace Engine {

    bool AbilitySystemComponent::ActivateAbility(size_t index)
    {
        if (index < m_Abilities.size())
        {
            Ref<Ability> ability = m_Abilities[index];
            if (ability->OnCooldown()) return false;
            ability->Activate();
            return true;
        }
        else
        {
            ENGINE_ERROR("Ability index out of range");
        }
        return false;
    }

    const std::vector<Ref<Ability>>& AbilitySystemComponent::GetAbilities() const
    {
        return m_Abilities;
    }
}

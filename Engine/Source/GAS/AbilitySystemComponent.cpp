#include "Enginepch.h"

#include "AbilitySystemComponent.h"
#include "Ability.h"

namespace Engine {

    bool AbilitySystemComponent::ActivateAbility(size_t index)
    {
        if (index >= m_Abilities.size())
        {
            ENGINE_ERROR("Ability index out of range");
            return false;
        }

        // Retrieve the entity and get the NativeScriptComponent
        Entity& abilityEntity = m_Abilities[index];
        auto& scriptComponent = abilityEntity.GetComponent<NativeScriptComponent>();

        // Get the bound ability script (e.g., Fireball)
        Ability* ability = static_cast<Ability*>(scriptComponent.Instance);

        if (ability && !ability->OnCooldown())
        {
            ability->Activate();
            return true;
        }

        return false;
    }

    const std::vector<Entity>& AbilitySystemComponent::GetAbilities() const
    {
        return m_Abilities;
    }
}

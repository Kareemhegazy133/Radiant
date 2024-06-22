#include "Enginepch.h"
#include "World/Entities/Entity.h"
#include "World/Entities/Ability.h"

#include "AbilitiesComponent.h"

namespace Engine {

    void AbilitiesComponent::ActivateAbility(size_t index, Entity& caster)
    {
        if (index < m_Abilities.size())
        {
            Ref<Ability> ability = m_Abilities[index];
            if (ability->OnCooldown()) return;
            ability->Activate(caster);
        }
        else
        {
            ENGINE_ERROR("Ability index out of range");
        }
    }
}

#include "Enginepch.h"
#include "World/Entities/Entity.h"
#include "World/Entities/Ability.h"

#include "AbilitiesComponent.h"

namespace Engine {

    void AbilitiesComponent::ActivateAbility(size_t index, Entity& caster)
    {
        if (index < m_Abilities.size())
        {
            m_Abilities[index]->Activate(caster);
        }
        else
        {
            ENGINE_ERROR("Ability index out of range");
        }
    }
}

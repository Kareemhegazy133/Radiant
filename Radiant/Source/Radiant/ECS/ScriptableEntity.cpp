#include "Radiant/rdpch.h"
#include "ScriptableEntity.h"

#include "GameplayLevel.h"

namespace Radiant {

	// Out of line, not inline in the header: returning GameplayLevel by value
	// needs the complete type, and GameplayLevel.h pulls in Level.h (and with
	// it GameApplication.h). Keeping that out of ScriptableEntity.h means every
	// script header stays cheap to include; a script that actually calls
	// through the handle gets the complete type from Radiant.h.
	GameplayLevel ScriptableEntity::GetLevel() const
	{
		return m_Entity.GetLevel();
	}

}
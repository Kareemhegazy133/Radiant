#include "Radiant/rdpch.h"
#include "GameplayLevel.h"

namespace Radiant {

	Level* GameplayLevel::Resolve(const char* verb) const
	{
		// A default-constructed handle has no level to route through, and so no
		// level that could warn about it — the one case that cannot be
		// delegated downward. Everything past this point (dead entity handles,
		// missing components) is Level's to report, and duplicating it here
		// would double every warning.
		if (!m_Level)
		{
			RADIANT_WARN("GameplayLevel: {0} called on a handle with no level", verb);
			return nullptr;
		}

		return m_Level;
	}

	// Every verb below is the same two lines, and that is the point: the guard
	// lives once in Resolve, and the behaviour lives once in Level. Adding a
	// verb costs one forwarder — never a copied guard block (playbook §4).
	//
	// The verb name travels as a string because it is what the WARN reports:
	// "which call did this" is the whole value of the log line.

	Entity GameplayLevel::CreateEntity(const std::string& name)
	{
		if (Level* level = Resolve("CreateEntity"))
			return level->CreateEntity(name);

		return {};
	}

	void GameplayLevel::DestroyEntity(Entity entity)
	{
		if (Level* level = Resolve("DestroyEntity"))
			level->DestroyEntity(entity);
	}

	void GameplayLevel::DestroyEntity(UUID entityID)
	{
		if (Level* level = Resolve("DestroyEntity"))
			level->DestroyEntity(entityID);
	}

	Entity GameplayLevel::FindEntityByName(std::string_view name)
	{
		if (Level* level = Resolve("FindEntityByName"))
			return level->FindEntityByName(name);

		return {};
	}

	Entity GameplayLevel::GetEntityByUUID(UUID uuid)
	{
		if (Level* level = Resolve("GetEntityByUUID"))
			return level->GetEntityByUUID(uuid);

		return {};
	}

	Level::CollisionCallbackHandle GameplayLevel::AddCollisionCallback(std::function<void(const Level::CollisionEvent&)> callback)
	{
		if (Level* level = Resolve("AddCollisionCallback"))
			return level->AddCollisionCallback(std::move(callback));

		// An invalid handle, so the caller's RemoveCollisionCallback in its
		// teardown path stays a harmless no-op rather than a special case
		return {};
	}

	void GameplayLevel::RemoveCollisionCallback(Level::CollisionCallbackHandle& handle)
	{
		if (Level* level = Resolve("RemoveCollisionCallback"))
			level->RemoveCollisionCallback(handle);
	}

	UUID GameplayLevel::GetUUID() const
	{
		if (Level* level = Resolve("GetUUID"))
			return level->GetUUID();

		return UUID(0);
	}

	const std::string& GameplayLevel::GetName() const
	{
		if (Level* level = Resolve("GetName"))
			return level->GetName();

		return NoLevelName;
	}

}
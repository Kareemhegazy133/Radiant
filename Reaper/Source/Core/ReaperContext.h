#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

#include "GameStateManager.h"

/** Content paths for each state's asset registry and the gameplay level. */
struct ReaperConfig
{
	std::string MainMenuAssetRegistryPath = "Assets/MainMenu.rdar";
	std::string GameplayAssetRegistryPath = "Assets/Gameplay.rdar";

	std::string LevelAssetPath = "Assets/Levels/Level.rdlvl";

};

/**
 * Process-wide game context, created once by the Reaper application: boots the
 * engine systems the game depends on (AssetManager), owns the GameStateManager,
 * holds the content-path configuration behind static accessors, and applies the
 * ImGui theme.
 */
class ReaperContext
{
public:
	ReaperContext();
	~ReaperContext();

	static const std::string& GetMainMenuAssetRegistryPath() { return s_Instance->GetConfig().MainMenuAssetRegistryPath; }
	static const std::string& GetGameplayAssetRegistryPath() { return s_Instance->GetConfig().GameplayAssetRegistryPath; }
	static const std::string& GetLevelAssetPath() { return s_Instance->GetConfig().LevelAssetPath; }

private:
	const ReaperConfig& GetConfig() const { return m_Config; }
	void SetGameTheme();

private:
	ReaperConfig m_Config;
	GameStateManager* m_StateManager = nullptr;

	inline static ReaperContext* s_Instance = nullptr;

};
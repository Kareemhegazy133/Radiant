#pragma once

#include <Radiant.h>

using namespace Radiant;

class MainMenuState : public GameState
{
public:
	virtual ~MainMenuState() = default;

	void OnEnter() override;

	void OnExit() override;

	void OnUpdate(Timestep ts) override;

	void OnRender() override;

	void OnEvent(Event& e) override;

private:
	Ref<Texture2D> m_BgTexture;

	const std::string& m_AssetRegistryPath = "Assets/MainMenu.rdar";
};
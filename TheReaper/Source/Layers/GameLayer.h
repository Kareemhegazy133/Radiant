#pragma once

#include <Radiant.h>

using namespace Radiant;

class GameLayer : public Layer
{
public:

	GameLayer();
	~GameLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;

	void OnEvent(Event& e) override;

private:
	void OnUpdateMainMenu(Timestep ts);
	void OnUpdateGameplay(Timestep ts);
	void OnUpdatePaused(Timestep ts);

	void OnRenderMainMenu();
	void OnRenderGameplay();
	void OnRenderPaused();

private:
	bool OnWindowResized(WindowResizeEvent& e);

// TEMP
private:
	void CreateDEBUG();
	void LoadDEBUG();
	void SaveDEBUG();

private:
	Ref<Level> m_Level;
	Ref<Framebuffer> m_Framebuffer;

	enum class GameState
	{
		MainMenu = 0, Gameplay = 1, Paused = 2
	};
	GameState m_GameState = GameState::Gameplay;

	// TEMP
	Ref<Texture2D> m_CheckerboardTexture;
	Ref<Texture2D> m_SpriteSheet;
	Ref<SubTexture2D> m_TextureStairs, m_TextureBarrel, m_TextureTree;
	Entity m_Camera;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
};

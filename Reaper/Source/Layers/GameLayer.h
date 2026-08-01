#pragma once

#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;

/**
 * Drives the gameplay Level: loads it (or debug-creates one when no registry
 * exists) on attach and re-binds the CameraController script (bindings are
 * code-only and don't survive level load). Simulation runs in OnFixedUpdate
 * (Level::OnFixedUpdate at the engine's fixed rate); OnUpdate only renders —
 * Level::OnRender into an offscreen framebuffer that OnImGuiRender presents
 * as a fullscreen ImGui image. Owns the Level and Framebuffer via Ref.
 * Escape pushes GamePausedState. Constructed by GameplayState::OnEnter;
 * deleted by the engine LayerStack after the deferred pop.
 */
class GameLayer : public Layer
{
public:
	GameLayer();
	~GameLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnFixedUpdate(Timestep ts) override;
	void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;

	void OnEvent(Event& e) override;

private:
	bool OnWindowResized(WindowResizeEvent& e);
	bool OnKeyPressed(KeyPressedEvent& e);

	// TEMP
private:
	void CreateDEBUG();
	void LoadDEBUG();

private:
	Ref<Framebuffer> m_Framebuffer;
	Ref<Level> m_Level;

	// RAD-29 verification scaffolding — the level-wide collision channel (the
	// one GAS will use). Removed in OnDetach: the Level owns the callback and
	// cannot tell when its subscriber dies. Retires with RAD-92.
	Level::CollisionObserverHandle m_CollisionObserver;

	// TEMP
	Entity m_Camera;
};
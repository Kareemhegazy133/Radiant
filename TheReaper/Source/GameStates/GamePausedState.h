#pragma once

#include <Radiant.h>

using namespace Radiant;

class GamePausedState : public GameState
{
public:
	virtual ~GamePausedState() = default;

	void OnEnter() override;

	void OnExit() override;

	void OnUpdate(Timestep ts) override;

	void OnRender() override;

	void OnEvent(Event& e) override;

};
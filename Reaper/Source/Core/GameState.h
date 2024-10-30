#pragma once

#include <Radiant.h>

class GameState : public RefCounted
{
public:
	virtual ~GameState() = default;

	// Called when the state is entered
	virtual void OnEnter() = 0;

	// Called when the state is exited
	virtual void OnExit() = 0;

	// Called every frame to update the state
	virtual void OnUpdate(Timestep ts) = 0;

	// Called every frame to render the state
	virtual void OnRender() = 0;

	// Called when an event occurs
	virtual void OnEvent(Event& e) = 0;
};

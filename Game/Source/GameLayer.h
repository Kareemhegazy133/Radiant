#pragma once

#include "Engine.h"

class GameLayer : public Engine::Layer
{
public:
	GameLayer();
	virtual ~GameLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Engine::Timestep ts) override;
	void OnEvent(Engine::Event& e) override;

};
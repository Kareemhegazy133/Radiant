#pragma once

#include "Engine.h"

#include "Level.h"

using namespace Engine;

class GameLayer : public Layer
{
public:
	GameLayer();
	virtual ~GameLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

private:
	Ref<Level> m_Level;
};


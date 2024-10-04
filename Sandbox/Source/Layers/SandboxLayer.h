#pragma once

#include <Radiant.h>

using namespace Radiant;

class SandboxLayer : public Layer
{
public:

	SandboxLayer();
	~SandboxLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Timestep ts) override;
	void OnEvent(Event& e) override;

private:
	bool OnWindowResized(WindowResizeEvent& e);

private:
	Ref<Level> m_Level;

	Ref<Texture2D> m_CheckerboardTexture;
	Ref<Texture2D> m_SpriteSheet;
	Ref<SubTexture2D> m_TextureStairs, m_TextureBarrel, m_TextureTree;
	Entity m_Camera;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
};


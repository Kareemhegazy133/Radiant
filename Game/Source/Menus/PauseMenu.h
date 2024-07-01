#pragma once

#include <Engine.h>

using namespace Engine;

class PauseMenu : public Menu
{
public:
	PauseMenu();
	virtual ~PauseMenu();

	void Initialize() override;

	void OnUpdate(Timestep ts) override;

	void OnResumeButtonClicked();
	void OnQuitButtonClicked();

	void Show() override;
	void Hide() override;
	bool IsVisible() const override;

private:
	TextComponent& m_TitleText = AddComponent<TextComponent>();
	//ButtonComponent& m_ResumeButton;
	//ButtonComponent& m_QuitButton;

	// TODO: Manage Game font elsewhere
	sf::Font m_Font;
};


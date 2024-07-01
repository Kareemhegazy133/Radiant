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
	TextComponent& titleText = AddComponent<TextComponent>();
	ButtonComponent& resumeButton = AddComponent<ButtonComponent>();
	//ButtonComponent& quitButton = AddComponent<ButtonComponent>();

	// TODO: Manage Game font elsewhere
	sf::Font m_Font;
};


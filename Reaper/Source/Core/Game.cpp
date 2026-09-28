#include <Radiant/Radiant.h>

// Reaper opts into the engine namespace (game-local choice; the engine no longer injects it)
using namespace Radiant;
#include <Radiant/Core/EntryPoint.h>

#include "ReaperContext.h"
#include "Layers/UILayer.h"

/**
 * The application: wires the game context and the persistent UILayer overlay.
 * Ownership of the pushed layer transfers to the engine LayerStack, which
 * deletes layers when they are popped or the stack is cleared.
 */
class Reaper : public Radiant::GameApplication
{
public:
	Reaper(const GameApplicationSpecification& specification)
		: GameApplication(specification)
	{
		PushOverlay(m_UILayer);
	}

	~Reaper()
	{
		PopOverlay(m_UILayer);
	}

private:
	ReaperContext gameContext;
	Layer* m_UILayer = new UILayer();
};

Radiant::GameApplication* Radiant::CreateGameApplication()
{
	GameApplicationSpecification specification;
	specification.Name = "Reaper";
	specification.WindowWidth = 1280;
	specification.WindowHeight = 720;
	specification.VSync = true;
	specification.IconPath = "Assets/Textures/ReaperIcon.jpg";
	specification.FontPath = "Assets/Fonts/Euljiro.ttf";
	specification.FontSize = 20.0f;

	return new Reaper(specification);
}

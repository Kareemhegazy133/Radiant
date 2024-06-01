#include "Level.h"

Level::Level(sf::RenderWindow* renderWindow)
    : World(renderWindow)
{
	/*std::vector<sf::IntRect> idleFrames;
	for (int i = 0; i < 18; i++)
	{
		idleFrames.push_back(sf::IntRect(902 * i + 1, 1, 902, 902));
	}

	auto& playerAnimationComponent = player.AddComponent<AnimationComponent>();
	playerAnimationComponent.AddAnimation("Idle", idleFrames, 0.05f);
	playerAnimationComponent.SetAnimation("Idle");*/
	
	auto platform = CreateEntity("Platform");
	platform.AddComponent<SpriteComponent>("Platform");
	auto& platform_transform = platform.GetComponent<TransformComponent>();
	platform_transform.setPosition(100.f, 600.f);

	auto& platform_rb2d = platform.AddComponent<Rigidbody2DComponent>();
	platform.AddComponent<BoxCollider2DComponent>();
}

Level::~Level()
{
}

void Level::OnUpdate(Timestep ts)
{
 

    World::OnUpdate(ts);
}

void Level::OnRender()
{

}

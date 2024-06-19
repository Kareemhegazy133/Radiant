#include "Level.h"

Level::Level()
    : World(static_cast<sf::RenderWindow*>(GameApplication::Get().GetWindow().GetNativeWindow()))
{
	auto platform = CreateEntity("Platform");
	platform.AddComponent<SpriteComponent>("Platform");
	auto& platform_transform = platform.GetComponent<TransformComponent>();
	platform_transform.setPosition(300.f, 600.f);

	auto& platform_rb2d = platform.AddComponent<Rigidbody2DComponent>();
	platform.AddComponent<BoxCollider2DComponent>();

	/*auto platform2 = CreateEntity("Platform2");
	platform2.AddComponent<SpriteComponent>("Platform");
	auto& platform2_transform = platform2.GetComponent<TransformComponent>();
	platform2_transform.setPosition(300.f, 0.f);

	auto& platform2_rb2d = platform2.AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
	platform2.AddComponent<BoxCollider2DComponent>();*/
}

Level::~Level()
{
}

void Level::OnUpdate(Timestep ts)
{
	//m_Player.OnUpdate(ts);

    World::OnUpdate(ts);
}

void Level::OnRender()
{

}

void Level::OnEvent(Event& e)
{

}

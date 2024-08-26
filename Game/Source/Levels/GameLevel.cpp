#include "GameLevel.h"

#include "Layers/GameLayer.h"

GameLevel::GameLevel()
	: m_CharacterInfoMenu(m_Player.GetComponent<AttributesComponent>(), m_Player.GetComponent<CharacterComponent>())
{
	std::cout << "GameLevel Created!" << std::endl;

	auto platform = CreateGameObject("Platform");
	platform.AddComponent<SpriteComponent>("Platform");
	auto& platform_transform = platform.GetComponent<TransformComponent>();
	platform_transform.SetPosition(300.f, 600.f);

	platform.AddComponent<Rigidbody2DComponent>();
	platform.AddComponent<BoxCollider2DComponent>();

}

GameLevel::~GameLevel()
{
	std::cout << "GameLevel Destroyed!" << std::endl;
}

void GameLevel::OnUpdate(Timestep ts)
{
	auto view = GetAllGameObjectsWith<MetadataComponent, TransformComponent, SpriteComponent, BoxCollider2DComponent>();
	for (auto entityHandle : view)
	{
		auto& [metadata, transform, sprite, bc2d] = view.get<
			MetadataComponent, TransformComponent, SpriteComponent, BoxCollider2DComponent>(entityHandle);

		// Skip inactive Entities
		if (!metadata.IsActive) continue;

		Entity entity(entityHandle, this);
		entity.OnUpdate(ts);

		if (metadata.Tag == "Player" && m_CharacterInfoMenu.IsVisible())
		{
			m_CharacterInfoMenu.UpdatePlayerInfo();
		}

		// Update Physics Colliders for all entities
		m_Physics.UpdateBoxColliderFixture(bc2d, transform, sprite);
	}

	// Apply Physics
	m_Physics.OnUpdate(ts);
}

void GameLevel::OnRender()
{
	// Update the transform after applying physics for sprite rendering
	auto view = GetAllGameObjectsWith<MetadataComponent, TransformComponent, SpriteComponent>();
	for (auto entity : view)
	{
		auto& [metadata, transform, sprite] = view.get<MetadataComponent, TransformComponent, SpriteComponent>(entity);

		// Skip inactive Entities
		if (!metadata.IsActive) continue;

		// Apply the transform to the sprite
		sprite.SetPosition(transform.GetPosition());
		sprite.SetRotation(transform.GetRotation());
		sprite.SetScale(
			{ transform.GetScale().x * sprite.GetScale().x,
			transform.GetScale().y * sprite.GetScale().y }
		);

		// Draw the sprite to the render window
		m_RenderWindow->draw(sprite);
	}

	// Render any active menus
	if (m_CharacterInfoMenu.IsVisible())
	{
		m_CharacterInfoMenu.OnRender();
	}
}

void GameLevel::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(GameLevel::OnKeyPressed));
	if (e.Handled) return;
	
	if (m_CharacterInfoMenu.IsVisible())
	{
		m_CharacterInfoMenu.OnEvent(e);
	}

	if (GameLayer::GetGameState() == GameLayer::GameState::Paused)
	{
		m_PauseMenu.OnEvent(e);
	}	
}

bool GameLevel::OnKeyPressed(KeyPressedEvent& e)
{
	if (e.GetKeyCode() == Key::Escape)
	{
		if (GameLayer::GetGameState() == GameLayer::GameState::Paused)
		{
			m_PauseMenu.SetVisibility(false);
			GameLayer::SetGameState(GameLayer::GameState::Playing);
		}
		else
		{
			m_PauseMenu.SetVisibility(true);
			GameLayer::SetGameState(GameLayer::GameState::Paused);
		}
		return true;
	}
	else if (e.GetKeyCode() == Key::C)
	{
		if (!m_CharacterInfoMenu.IsVisible())
		{
			m_CharacterInfoMenu.SetVisibility(true);
		}
		else
		{
			m_CharacterInfoMenu.SetVisibility(false);
		}
		return true;
	}

	if (e.GetKeyCode() == Key::X)
	{
		SceneSerializer serializer(&Scene::GetScene());
		serializer.Serialize("Saves/GameLevel.sav");
		return true;
	}

	return false;
}

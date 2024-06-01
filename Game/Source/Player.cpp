#include "Player.h"

using namespace Engine;

Player::Player()
    : Character("Player")
{
    auto& transform = GetComponent<TransformComponent>();
    transform.setPosition(m_Position);

    AddComponent<SpriteComponent>("Player");
    AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    AddComponent<BoxCollider2DComponent>();
}

Player::~Player()
{
}

void Player::OnUpdate(Timestep ts)
{
    
}
#include "Player.h"

using namespace Engine;

Player::Player()
    : Character("Player", "Player", sf::Vector2f(300.f, 200.f))
{
    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 32;
    m_FrameHeightPadding = 40;
    m_FrameCount = 18;

	auto& playerAnimationComponent = AddComponent<AnimationComponent>();
	SetupAnimation("Idle", m_FrameCount, m_FrameWidth, m_FrameHeight, m_FrameWidthPadding, m_FrameHeightPadding, 0.05f);
	playerAnimationComponent.SetAnimation("Idle");

    auto& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    AddComponent<BoxCollider2DComponent>();
}

Player::~Player()
{
}

void Player::OnUpdate(Timestep ts)
{
    
}

void Player::OnCollisionBegin(const Entity& other)
{

}

void Player::OnCollisionEnd(const Entity& other)
{

}

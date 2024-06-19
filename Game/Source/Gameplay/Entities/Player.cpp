#include "Player.h"
#include "Gameplay/Attributes.h"
#include "Gameplay/Entities/Abilities.h"

using namespace Engine;

Player::Player()
    : Character("Player", "Player", sf::Vector2f(300.f, 200.f))
{
    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 32;
    m_FrameHeightPadding = 40;
    m_FrameCount = 18;

    m_Speed = 200.f;

	auto& playerAnimationComponent = AddComponent<AnimationComponent>();
	SetupAnimation("Idle", m_FrameCount, m_FrameWidth, m_FrameHeight, m_FrameWidthPadding, m_FrameHeightPadding, 0.05f, true);
	playerAnimationComponent.SetAnimation("Idle");

    auto& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Player::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Player::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    auto& attributes = AddComponent<AttributesComponent>();
    attributes.SetAttribute(Attributes::Health, 100.f);
    attributes.SetAttribute(Attributes::Stamina, 100.f);
    attributes.SetAttribute(Attributes::Strength, 10.f);
    attributes.SetAttribute(Attributes::Defense, 10.f);
    attributes.SetAttribute(Attributes::Magic, 10.f);

    GAME_INFO("Health: {0}", attributes.GetAttribute(Attributes::Health));

    auto& abilities = AddComponent<AbilitiesComponent>();
    abilities.AddAbility<Fireball>();

    GetComponent<MetadataComponent>().OnUpdate = BIND_MEMBER_FUNCTION(Player::OnUpdate, this);
}

Player::~Player()
{
}

void Player::OnUpdate(Timestep ts)
{
    // Reset velocity each frame
    sf::Vector2f velocity = { 0.0f, 0.0f };

    if (Input::IsKeyPressed(Key::W))
        velocity.y -= m_Speed;
    if (Input::IsKeyPressed(Key::S))
        velocity.y += m_Speed;
    if (Input::IsKeyPressed(Key::A))
        velocity.x -= m_Speed;
    if (Input::IsKeyPressed(Key::D))
        velocity.x += m_Speed;

    auto& transform = GetComponent<TransformComponent>();

    transform.setPosition(
        transform.getPosition().x + velocity.x * ts,
        transform.getPosition().y + velocity.y * ts
    );

    if (Input::IsKeyPressed(Key::Num1))
        GetComponent<AbilitiesComponent>().ActivateAbility(0, *this);
}

void Player::OnCollisionBegin(Entity& other)
{
    //GAME_INFO("Player collided with: {0}", other.GetComponent<TagComponent>().Tag);
}

void Player::OnCollisionEnd(Entity& other)
{

}

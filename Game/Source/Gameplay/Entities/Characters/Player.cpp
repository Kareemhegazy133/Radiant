#include "Player.h"
#include "Gameplay/Attributes.h"
#include "Gameplay/Entities/Abilities/Abilities.h"
#include "Gameplay/Entities/AnimationStates.h"

using namespace Engine;

Player::Player()
    : Character("Player", sf::Vector2f(300.f, 200.f))
{
    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 60;
    m_FrameHeightPadding = 40;

	SetupAnimation(CharacterAnimationState::Idle,
        "PlayerIdle", 
        18, 
        m_FrameWidth, 
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.05f,
        true
    );
    SetupAnimation(CharacterAnimationState::Walk,
        "PlayerWalk",
        24,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.05f,
        true
    );
	animation.SetAnimation(CharacterAnimationState::Idle);

    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Player::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Player::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    attributes.SetAttribute(Attributes::Health, 100.f, 10);
    attributes.SetAttribute(Attributes::Stamina, 100.f, 5);
    attributes.SetAttribute(Attributes::Strength, 10.f, 4);
    attributes.SetAttribute(Attributes::Defense, 10.f, 7);
    attributes.SetAttribute(Attributes::Magic, 10.f, 3);

    GAME_INFO("Health: {0}", attributes.GetAttribute(Attributes::Health));

    abilities.AddAbility<Fireball>();

    metadata.OnUpdate = BIND_MEMBER_FUNCTION(Player::OnUpdate, this);

    // Starting Stats
    character.Level = 1;
    attributes.AddAttributePoints(4);
    character.Coins = 0;
    character.Diamonds = 0;
    character.CurrentHealth = attributes.GetAttribute(Attributes::Health);
    character.CurrentStamina = attributes.GetAttribute(Attributes::Stamina);
    character.Speed = 200.f;
}

Player::~Player()
{
}

void Player::OnUpdate(Timestep ts)
{
    animation.Update(ts);

    // Reset velocity each frame
    sf::Vector2f velocity = { 0.0f, 0.0f };

    if (Input::IsKeyPressed(Key::W))
        velocity.y -= character.Speed;
    if (Input::IsKeyPressed(Key::S))
        velocity.y += character.Speed;
    if (Input::IsKeyPressed(Key::A))
        velocity.x -= character.Speed;
    if (Input::IsKeyPressed(Key::D))
        velocity.x += character.Speed;

    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );

    // TODO: Add Keybinds functionality
    if (Input::IsKeyPressed(Key::Num1))
        abilities.ActivateAbility(0, *this);
}

void Player::OnCollisionBegin(Entity& other)
{
    //GAME_INFO("Player collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
}

void Player::OnCollisionEnd(Entity& other)
{

}

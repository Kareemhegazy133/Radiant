#include "Player.h"
#include "Gameplay/Attributes.h"
#include "Gameplay/Entities/Abilities/Abilities.h"

using namespace Engine;

Player::Player()
    : Character("Player", sf::Vector2f(300.f, 200.f))
{
    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 60;
    m_FrameHeightPadding = 40;

    SetupAnimations();
    SetState(CharacterState::Idle);

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

void Player::SetupAnimations()
{
    SetupAnimation(CharacterState::Idle,
        "PlayerIdle",
        18,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        true
    );

    SetupAnimation(CharacterState::Walk,
        "PlayerWalk",
        24,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        true
    );

    SetupAnimation(CharacterState::Throw,
        "PlayerThrow",
        12,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.1f,
        false
    );
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

    if (velocity.x != 0.f)
    {
        SetState(CharacterState::Walk);
    }
    else {
        SetState(CharacterState::Idle);
    }

    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );

    // TODO: Add Keybinds functionality
    if (Input::IsKeyPressed(Key::Num1))
    {
        bool casted = abilities.ActivateAbility(0, *this);
        if (casted)
        {
            // TODO: Stay on this state till throw animation is done
            SetState(CharacterState::Throw);
        }
    }
}

void Player::SetState(CharacterState newState)
{
    if (newState == m_CurrentState) return;

    OnExitState(m_CurrentState);

    m_CurrentState = newState;

    OnEnterState(m_CurrentState);
}

void Player::OnEnterState(CharacterState state)
{
    switch (state)
    {
    case CharacterState::Idle:
        animation.SetAnimation(CharacterState::Idle);
        break;
    case CharacterState::Walk:
        animation.SetAnimation(CharacterState::Walk);
        break;
    case CharacterState::Throw:
        animation.SetAnimation(CharacterState::Throw);
        break;
    }
}

void Player::OnExitState(CharacterState state)
{
}

void Player::OnCollisionBegin(Entity& other)
{
    //GAME_INFO("Player collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
}

void Player::OnCollisionEnd(Entity& other)
{

}

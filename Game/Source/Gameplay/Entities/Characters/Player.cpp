#include "Player.h"

#include "GameContext.h"

#include "Gameplay/Entities/Abilities/Abilities.h"

using namespace Engine;

void Player::OnCreate()
{
    Character::OnCreate();

    // TODO: Revisit maybe?
    m_CharacterInfoMenu = new CharacterInfoMenu(this);
    m_AttributeSet = new GameAttributeSet();

    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 60;
    m_FrameHeightPadding = 40;

    auto& transform = GetComponent<TransformComponent>();

    transform.SetPosition({ 300.f, 200.f });

    SetupAnimations();
    SetupStateMachine();

    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Player::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Player::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    m_AttributeSet->SetAttribute(Attributes::Health, 100.f, 10);
    m_AttributeSet->SetAttribute(Attributes::Stamina, 100.f, 5);
    m_AttributeSet->SetAttribute(Attributes::Strength, 10.f, 4);
    m_AttributeSet->SetAttribute(Attributes::Defense, 10.f, 7);
    m_AttributeSet->SetAttribute(Attributes::Magic, 10.f, 3);

    GAME_INFO("Player Health: {0}", m_AttributeSet->GetAttribute(Attributes::Health));

    auto& abilities = GetComponent<AbilitySystemComponent>();
    abilities.AddAbility<Fireball>("Fireball", this, GameContext::GetLevel());
    

    // Starting Stats
    Level = 1;
    m_AttributeSet->AddAttributePoints(4);
    Coins = 0;
    Diamonds = 0;
    CurrentHealth = m_AttributeSet->GetAttribute(Attributes::Health);
    CurrentStamina = m_AttributeSet->GetAttribute(Attributes::Stamina);
    Speed = 200.f;

    Direction.x = 1.f;
}

void Player::OnUpdate(Timestep ts)
{
    auto& animation = GetComponent<AnimationComponent>();
    animation.Update(ts);

    // Reset velocity each frame
    sf::Vector2f velocity = { 0.0f, 0.0f };

    if (Input::IsKeyPressed(Key::W))
        velocity.y -= Speed;
    if (Input::IsKeyPressed(Key::S))
        velocity.y += Speed;
    if (Input::IsKeyPressed(Key::A))
        velocity.x -= Speed;
    if (Input::IsKeyPressed(Key::D))
        velocity.x += Speed;

    if (velocity.x != 0.f)
    {
        m_StateMachine.SetState(PlayerState::Walking);

        auto& sprite = GetComponent<SpriteComponent>();
        // Flip the sprite based on direction
        if (velocity.x < 0.f)
        {
            sprite.SetScale(-1.f, 1.f); // Turn left
            Direction.x = -1.f;
        }
        else if (velocity.x > 0.f)
        {
            sprite.SetScale(1.f, 1.f); // Turn right
            Direction.x = 1.f;
        }
    }
    else {
        m_StateMachine.SetState(PlayerState::Idle);
    }

    auto& transform = GetComponent<TransformComponent>();
    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );

    // TODO: Add Keybinds functionality
    if (Input::IsKeyPressed(Key::Num1))
    {
        auto& abilities = GetComponent<AbilitySystemComponent>();
        bool casted = abilities.ActivateAbility(0);
        if (casted)
        {
            m_StateMachine.SetState(PlayerState::Throwing);
        }
    }

    m_StateMachine.Update();
}

void Player::OnDestroy()
{
    delete m_CharacterInfoMenu;
    m_CharacterInfoMenu = nullptr;

    delete m_AttributeSet;
    m_AttributeSet = nullptr;
}

void Player::SetupAnimations()
{
    SetupAnimation(PlayerState::Idle,
        "PlayerIdle",
        18,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        true
    );

    SetupAnimation(PlayerState::Walking,
        "PlayerWalk",
        24,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        true
    );

    SetupAnimation(PlayerState::Throwing,
        "PlayerThrow",
        12,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding - 15,
        m_FrameHeightPadding,
        0.05f,
        false
    );
}

void Player::SetupStateMachine()
{
    m_StateMachine.AddState(PlayerState::Idle,
        [this]() {
            auto& animation = GetComponent<AnimationComponent>();
            // OnEnter Idle State
            animation.SetAnimation(PlayerState::Idle);
        },
        [this]() {
            // OnUpdate Idle State
            // You can add Idle-specific logic here
        },
        []() {
            // OnExit Idle State
        },
        false
        );

    m_StateMachine.AddState(PlayerState::Walking,
        [this]() {
            auto& animation = GetComponent<AnimationComponent>();
            // OnEnter Walking State
            animation.SetAnimation(PlayerState::Walking);
        },
        [this]() {
            // OnUpdate Walking State
            // You can add Walking-specific logic here
        },
        []() {
            // OnExit Walking State
        },
        false
        );

    m_StateMachine.AddState(PlayerState::Throwing,
        [this]() {
            auto& animation = GetComponent<AnimationComponent>();
            // OnEnter Throwing State
            animation.SetAnimation(PlayerState::Throwing);
        },
        [this]() {
            auto& animation = GetComponent<AnimationComponent>();
            // OnUpdate Throwing State
            // Check if the throw animation is done
            if (animation.IsFinished())
            {
                // Transition back to Idle state once the animation is done
                m_StateMachine.SetLocked(false);
                m_StateMachine.SetState(PlayerState::Idle);
            }
        },
        []() {
            // OnExit Throwing State
        },
        true
        );

    m_StateMachine.SetState(PlayerState::Idle); // Start with Idle state
}

void Player::OnCollisionBegin(Entity& other)
{
    //GAME_INFO("Player collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
}

void Player::OnCollisionEnd(Entity& other)
{

}

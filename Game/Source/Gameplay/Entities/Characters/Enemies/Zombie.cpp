#include "Zombie.h"

#include "GameContext.h"

#include "Gameplay/Entities/Abilities/Abilities.h"

using namespace Engine;

void Zombie::OnCreate()
{
    Character::OnCreate();

    m_AttributeSet = new GameAttributeSet();

    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 60;
    m_FrameHeightPadding = 40;

    SetupAnimations();

    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Dynamic);
    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Zombie::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Zombie::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    m_AttributeSet->SetAttribute(Attributes::Health, 100.f, 10);
    m_AttributeSet->SetAttribute(Attributes::Stamina, 100.f, 5);
    m_AttributeSet->SetAttribute(Attributes::Strength, 10.f, 4);
    m_AttributeSet->SetAttribute(Attributes::Defense, 10.f, 7);
    m_AttributeSet->SetAttribute(Attributes::Magic, 10.f, 3);

    GAME_INFO("Zombie Health: {0}", m_AttributeSet->GetAttribute(Attributes::Health));

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

void Zombie::OnUpdate(Timestep ts)
{
    auto& animation = GetComponent<AnimationComponent>();
    animation.Update(ts);

    // Reset velocity each frame
    glm::vec2 velocity = { 0.0f, 0.0f };

    if (velocity.x != 0.f)
    {
        // Walking State

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
        // Idle State
    }

    auto& transform = GetComponent<TransformComponent>();
    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );
}

void Zombie::OnDestroy()
{
    delete m_AttributeSet;
    m_AttributeSet = nullptr;
}

void Zombie::SetupAnimations()
{
    SetupAnimation(EnemyState::Idle,
        "ZombieIdle",
        18,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        true
    );

    SetupAnimation(EnemyState::Walking,
        "ZombieWalk",
        24,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        true
    );

    SetupAnimation(EnemyState::Throwing,
        "ZombieThrow",
        12,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding - 15,
        m_FrameHeightPadding,
        0.05f,
        false
    );
}

void Zombie::OnCollisionBegin(Entity& other)
{
    //GAME_INFO("Zombie collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
}

void Zombie::OnCollisionEnd(Entity& other)
{

}

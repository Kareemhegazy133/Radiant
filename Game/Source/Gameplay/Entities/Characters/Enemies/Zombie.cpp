#include "Zombie.h"
#include "Gameplay/Attributes.h"
#include "Gameplay/Entities/Abilities/Abilities.h"

using namespace Engine;

Zombie::Zombie()
    : Character("Zombie", sf::Vector2f(500.f, 200.f))
{
    m_FrameWidth = 256;
    m_FrameHeight = 256;
    m_FrameWidthPadding = 60;
    m_FrameHeightPadding = 40;

    SetupAnimations();

    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Zombie::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Zombie::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    attributes.SetAttribute(Attributes::Health, 100.f, 10);
    attributes.SetAttribute(Attributes::Stamina, 100.f, 5);
    attributes.SetAttribute(Attributes::Strength, 10.f, 4);
    attributes.SetAttribute(Attributes::Defense, 10.f, 7);
    attributes.SetAttribute(Attributes::Magic, 10.f, 3);

    GAME_INFO("Zombie Health: {0}", attributes.GetAttribute(Attributes::Health));

    abilities.AddAbility<Fireball>();

    metadata.OnUpdate = BIND_MEMBER_FUNCTION(Zombie::OnUpdate, this);

    // Starting Stats
    character.Level = 1;
    attributes.AddAttributePoints(4);
    character.Coins = 0;
    character.Diamonds = 0;
    character.CurrentHealth = attributes.GetAttribute(Attributes::Health);
    character.CurrentStamina = attributes.GetAttribute(Attributes::Stamina);
    character.Speed = 200.f;

    character.Direction.x = 1.f;
}

Zombie::~Zombie()
{
}

void Zombie::OnUpdate(Timestep ts)
{
    animation.Update(ts);

    // Reset velocity each frame
    sf::Vector2f velocity = { 0.0f, 0.0f };

    if (velocity.x != 0.f)
    {
        // Walking State

        // Flip the sprite based on direction
        if (velocity.x < 0.f)
        {
            sprite.SetScale(-1.f, 1.f); // Turn left
            character.Direction.x = -1.f;
        }
        else if (velocity.x > 0.f)
        {
            sprite.SetScale(1.f, 1.f); // Turn right
            character.Direction.x = 1.f;
        }
    }
    else {
        // Idle State
    }

    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );
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

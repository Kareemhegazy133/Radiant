#include "Fireball.h"

#include "Levels/GameLevel.h"

#include "Gameplay/Entities/EntityStates.h"

using namespace Engine;

Fireball::Fireball()
    : Ability("Fireball")
{
    m_FrameWidth = 128;
    m_FrameHeight = 128;
    m_FrameWidthPadding = 20;
    m_FrameHeightPadding = 30;

    ability.Speed = 200.f;
    ability.MaxDuration = 2.f;
    ability.Cooldown = 4.f;
    ability.LastActivatedTime = -ability.Cooldown;

    SetupAnimation(AbilityState::Active,
        "FireballActive",
        41,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.025f,
        false
    );
    animation.SetAnimation(AbilityState::Active);

    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Fireball::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Fireball::OnCollisionEnd, this);

    metadata.IsActive = false;
    metadata.OnUpdate = BIND_MEMBER_FUNCTION(Fireball::OnUpdate, this);
    GAME_INFO("Fireball Constructed!");
}

Fireball::~Fireball()
{
}

void Fireball::Activate(Entity& caster)
{
    ability.LastActivatedTime = GameApplication::Get().SFMLGetTime();
    ability.Caster = &caster;
    if (metadata.IsActive) return;

    auto& casterTransform = caster.GetComponent<TransformComponent>();
    transform.SetPosition(
        casterTransform.GetPosition() + m_SocketOffset
    );
    
    metadata.IsActive = true;
    CreatePhysicsBoxCollider();
    GAME_INFO("Fireball Activated!");
}

void Fireball::Deactivate()
{
    metadata.IsActive = false;
    DestroyPhysicsBoxCollider();
    m_ActiveDuration = 0.0f;
    GAME_INFO("Fireball Deactivated!");
}

void Fireball::OnUpdate(Timestep ts)
{
    animation.Update(ts);

    // Update timer
    m_ActiveDuration += ts;
    if (m_ActiveDuration >= ability.MaxDuration) {
        Deactivate();
        return;
    }

    sf::Vector2f velocity = { 0.0f, 0.0f };
    velocity.x += ability.Speed;

    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );
}

void Fireball::OnCollisionBegin(Entity& other)
{
    GAME_INFO("Fireball collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
    Deactivate();
}

void Fireball::OnCollisionEnd(Entity& other)
{

}

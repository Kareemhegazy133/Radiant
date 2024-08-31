#include "Fireball.h"

using namespace Engine;

void Fireball::OnCreate()
{
    m_FrameWidth = 128;
    m_FrameHeight = 128;
    m_FrameWidthPadding = 20;
    m_FrameHeightPadding = 30;

    Speed = 200.f;
    MaxDuration = 2.f;
    Cooldown = 4.f;
    LastActivatedTime = -Cooldown;

    SetupAnimation(AbilityState::Active,
        "FireballActive",
        41,
        m_FrameWidth,
        m_FrameHeight,
        m_FrameWidthPadding,
        m_FrameHeightPadding,
        0.045f,
        false
    );
    animation.SetAnimation(AbilityState::Active);

    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Fireball::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Fireball::OnCollisionEnd, this);

    metadata.IsActive = false;
    GAME_INFO("Fireball Created!");
}

void Fireball::OnUpdate(Timestep ts)
{
    animation.Update(ts);

    // Update timer
    m_ActiveDuration += ts;
    if (m_ActiveDuration >= MaxDuration) {
        Deactivate();
        return;
    }

    sf::Vector2f velocity = { 0.0f, 0.0f };
    velocity.x += Speed * m_Direction.x;

    // Flip the sprite based on direction
    sprite.SetScale(m_Direction.x, 1.f);

    transform.SetPosition(
        transform.GetPosition().x + velocity.x * ts,
        transform.GetPosition().y + velocity.y * ts
    );
}

void Fireball::OnDestroy()
{

}

void Fireball::Activate()
{
    LastActivatedTime = GameApplication::Get().SFMLGetTime();
    if (metadata.IsActive) return;

    // TODO: 
    //m_Direction = caster.GetComponent<CharacterComponent>().Direction;
    m_Direction = { 1.f, 0.f };

    if ((m_SocketOffset.x < 0.f && m_Direction.x > 0.f) || (m_SocketOffset.x > 0.f && m_Direction.x < 0.f))
    {
        m_SocketOffset.x *= -1.f;
    }

    auto& casterTransform = Caster->GetComponent<TransformComponent>();
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
    animation.ResetAnimation();
    GAME_INFO("Fireball Deactivated!");
}

void Fireball::OnCollisionBegin(Entity& other)
{
    GAME_INFO("Fireball collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
    Deactivate();
}

void Fireball::OnCollisionEnd(Entity& other)
{

}

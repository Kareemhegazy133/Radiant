#include "Fireball.h"

using namespace Engine;

Fireball::Fireball()
    : Ability("Fireball", "Fireball")
{
    m_FrameWidth = 128;
    m_FrameHeight = 128;
    m_FrameWidthPadding = 0;
    m_FrameHeightPadding = 0;
    m_FrameCount = 41;

    ability.Speed = 200.f;

    SetupAnimation("Fireball", m_FrameCount, m_FrameWidth, m_FrameHeight, m_FrameWidthPadding, m_FrameHeightPadding, 0.025f, false);
    animation.SetAnimation("Fireball");

    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Fireball::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Fireball::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    metadata.IsActive = false;
    metadata.OnUpdate = BIND_MEMBER_FUNCTION(Fireball::OnUpdate, this);
    GAME_INFO("Fireball Constructed!");
}

Fireball::~Fireball()
{
}

void Fireball::Activate(Entity& caster)
{
    // TODO: Add Cooldown functionality
    ability.Caster = &caster;
    if (metadata.IsActive) return;

    auto& casterTransform = caster.GetComponent<TransformComponent>();
    transform.setPosition(
        casterTransform.getPosition().x + 75.f,
        casterTransform.getPosition().y + 50.f
    );
    
    metadata.IsActive = true;
    GAME_INFO("Fireball Activated!");
}

void Fireball::Deactivate()
{
    metadata.IsActive = false;
    ability.ActiveDuration = 0.0f;
    GAME_INFO("Fireball Deactivated!");
}

void Fireball::OnUpdate(Timestep ts)
{
    // Update timer
    ability.ActiveDuration += ts;
    if (ability.ActiveDuration >= ability.MaxDuration) {
        Deactivate();
        return;
    }

    sf::Vector2f velocity = { 0.0f, 0.0f };
    velocity.x += ability.Speed;

    transform.setPosition(
        transform.getPosition().x + velocity.x * ts,
        transform.getPosition().y + velocity.y * ts
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

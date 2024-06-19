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

    m_Speed = 200.f;
    m_ActiveDuration = 2.f;

    auto& fireballAnimationComponent = AddComponent<AnimationComponent>();
    SetupAnimation("Fireball", m_FrameCount, m_FrameWidth, m_FrameHeight, m_FrameWidthPadding, m_FrameHeightPadding, 0.025f, false);
    fireballAnimationComponent.SetAnimation("Fireball");

    auto& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Kinematic);
    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Fireball::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Fireball::OnCollisionEnd, this);

    AddComponent<BoxCollider2DComponent>();

    GetComponent<MetadataComponent>().IsActive = false;
    GetComponent<MetadataComponent>().OnUpdate = BIND_MEMBER_FUNCTION(Fireball::OnUpdate, this);
    GAME_INFO("Fireball Constructed!");
}

Fireball::~Fireball()
{
}

void Fireball::Activate(Entity& caster)
{
    if (GetComponent<MetadataComponent>().IsActive) return;

    auto& casterTransform = caster.GetComponent<TransformComponent>();
    auto& fireballTransform = GetComponent<TransformComponent>();
    fireballTransform.setPosition(
        casterTransform.getPosition().x + 50.f,
        casterTransform.getPosition().y
    );
    
    GetComponent<MetadataComponent>().IsActive = true;
    GAME_INFO("Fireball Activated!");
}

void Fireball::Deactivate()
{
    GetComponent<MetadataComponent>().IsActive = false;
    m_Timer = 0.0f;
    GAME_INFO("Fireball Deactivated!");
}

void Fireball::OnUpdate(Timestep ts)
{
    // Update timer
    m_Timer += ts;
    if (m_Timer >= m_ActiveDuration) {
        Deactivate();
        return;
    }

    sf::Vector2f velocity = { 0.0f, 0.0f };
    velocity.x += m_Speed;

    auto& transform = GetComponent<TransformComponent>();

    transform.setPosition(
        transform.getPosition().x + velocity.x * ts,
        transform.getPosition().y + velocity.y * ts
    );
}

void Fireball::OnCollisionBegin(Entity& other)
{
    // TODO: Ignore Caster
    if (other.GetComponent<MetadataComponent>().Tag == "Player") return;
    GAME_INFO("Fireball collided with: {0}", other.GetComponent<MetadataComponent>().Tag);
    Deactivate();
}

void Fireball::OnCollisionEnd(Entity& other)
{

}

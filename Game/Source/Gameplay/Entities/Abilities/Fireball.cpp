#include "Fireball.h"

#include "Gameplay/Entities/Characters/Character.h"

using namespace Engine;

void Fireball::OnCreate()
{
    Ability::OnCreate();

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

    auto& animation = GetComponent<AnimationComponent>();
    animation.SetAnimation(AbilityState::Active);

    Rigidbody2DComponent& rb2d = AddComponent<Rigidbody2DComponent>(Rigidbody2DComponent::BodyType::Kinematic);
    rb2d.OnCollisionBegin = BIND_MEMBER_FUNCTION(Fireball::OnCollisionBegin, this);
    rb2d.OnCollisionEnd = BIND_MEMBER_FUNCTION(Fireball::OnCollisionEnd, this);

    auto& metadata = GetComponent<MetadataComponent>();
    metadata.IsActive = false;
    GAME_INFO("Fireball Created!");
}

void Fireball::OnUpdate(Timestep ts)
{
    auto& animation = GetComponent<AnimationComponent>();
    animation.Update(ts);

    // Update timer
    m_ActiveDuration += ts;
    if (m_ActiveDuration >= MaxDuration) {
        Deactivate();
        return;
    }

    sf::Vector2f velocity = { 0.0f, 0.0f };
    velocity.x += Speed * m_Direction.x;

    auto& sprite = GetComponent<SpriteComponent>();
    // Flip the sprite based on direction
    sprite.SetScale(m_Direction.x, 1.f);

    auto& transform = GetComponent<TransformComponent>();
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
    auto& metadata = GetComponent<MetadataComponent>();
    if (metadata.IsActive) return;

    // TODO: Find a better way to access Character/Ability variables or a way to cast to the actual type automatically.
    if (Character* caster = static_cast<Character*>(Caster))
    {
        m_Direction = caster->Direction;
    }
    else
    {
        m_Direction = { 1.f, 0.f };
    }
    
    if ((m_SocketOffset.x < 0.f && m_Direction.x > 0.f) || (m_SocketOffset.x > 0.f && m_Direction.x < 0.f))
    {
        m_SocketOffset.x *= -1.f;
    }

    auto& casterTransform = Caster->GetComponent<TransformComponent>();

    auto& transform = GetComponent<TransformComponent>();
    transform.SetPosition(
        casterTransform.GetPosition() + m_SocketOffset
    );

    metadata.IsActive = true;
    CreatePhysicsBoxCollider();
    GAME_INFO("Fireball Activated!");
}

void Fireball::Deactivate()
{
    auto& metadata = GetComponent<MetadataComponent>();
    metadata.IsActive = false;
    DestroyPhysicsBoxCollider();
    m_ActiveDuration = 0.0f;

    auto& animation = GetComponent<AnimationComponent>();
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

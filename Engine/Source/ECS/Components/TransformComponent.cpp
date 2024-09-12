#include "Enginepch.h"
#include "TransformComponent.h"
#include "box2d/b2_body.h"

namespace Engine {

    TransformComponent::TransformComponent(const glm::vec2& position)
        : Transform()
    {
        SetPosition(position);
    }

    void TransformComponent::SetPosition(const glm::vec2& position)
    {
        if (RuntimeBody)
        {
            b2Body* body = static_cast<b2Body*>(RuntimeBody);
            body->SetTransform(b2Vec2(position.x, position.y), DEG_TO_RAD(GetRotation()));
        }
        Transform.setPosition(position.x, position.y);
    }

    void TransformComponent::SetPosition(float x, float y)
    {
        if (RuntimeBody)
        {
            b2Body* body = static_cast<b2Body*>(RuntimeBody);
            body->SetTransform(b2Vec2(x, y), DEG_TO_RAD(GetRotation()));
        }
        Transform.setPosition(x, y);
    }

    glm::vec2 TransformComponent::GetPosition()
    {
        if (RuntimeBody)
        {
            b2Body* body = static_cast<b2Body*>(RuntimeBody);
            const auto& position = body->GetPosition();
            Transform.setPosition(position.x, position.y);
        }
        auto position = Transform.getPosition();
        return glm::vec2(position.x, position.y);
    }

    void TransformComponent::SetRotation(float angle)
    {
        if (RuntimeBody)
        {
            b2Body* body = static_cast<b2Body*>(RuntimeBody);
            body->SetTransform(b2Vec2(GetPosition().x, GetPosition().y), DEG_TO_RAD(angle));
        }
        Transform.setRotation(angle);
    }

    float TransformComponent::GetRotation()
    {
        if (RuntimeBody) {
            b2Body* body = static_cast<b2Body*>(RuntimeBody);
            Transform.setRotation(RAD_TO_DEG(body->GetAngle()));
        }
        return Transform.getRotation();
    }

    void TransformComponent::SetScale(const glm::vec2& scale)
    {
        Transform.setScale(scale.x, scale.y);
    }

    void TransformComponent::SetScale(float scaleX, float scaleY)
    {
        Transform.setScale(scaleX, scaleY);
    }

    glm::vec2 TransformComponent::GetScale() const
    {
        auto scale = Transform.getScale();
        return glm::vec2(scale.x, scale.y);
    }

    void TransformComponent::SetOrigin(float x, float y)
    {
        Transform.setOrigin(x, y);
    }

    glm::vec2 TransformComponent::GetOrigin() const
    {
        auto origin = Transform.getOrigin();
        return glm::vec2(origin.x, origin.y);
    }

    void TransformComponent::Move(float offsetX, float offsetY)
    {
        Transform.move(offsetX, offsetY);
    }

    void TransformComponent::Rotate(float angle)
    {
        Transform.rotate(angle);
    }

    void TransformComponent::Scale(float factorX, float factorY)
    {
        Transform.scale(factorX, factorY);
    }

    sf::Transform TransformComponent::GetTransform() const
    {
        return Transform.getTransform();
    }

    void TransformComponent::Reset()
    {
        Transform = sf::Transformable();
    }
}

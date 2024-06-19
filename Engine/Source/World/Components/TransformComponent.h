#pragma once

#include "SFML/Graphics/Transformable.hpp"
#include "box2d/b2_body.h"

#include "World/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class TransformComponent : public Component
    {
    public:
        sf::Transformable Transform;

        // Storage for runtime b2Body
        void* RuntimeBody = nullptr;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const sf::Vector2f& position) : Transform()
        {
            setPosition(position);
        }

        operator const sf::Transformable& () { return Transform; }

        void setPosition(const sf::Vector2f& position) {
            if (RuntimeBody)
            {
                b2Body* body = static_cast<b2Body*>(RuntimeBody);
                body->SetTransform(b2Vec2(position.x, position.y), DEG_TO_RAD(getRotation()));
            }
            
            Transform.setPosition(position);
        }

        void setPosition(float x, float y) {
            if (RuntimeBody)
            {
                b2Body* body = static_cast<b2Body*>(RuntimeBody);
                body->SetTransform(b2Vec2(x, y), DEG_TO_RAD(getRotation()));
            }
            Transform.setPosition(x, y);
        }

        sf::Vector2f getPosition() {
            if (RuntimeBody)
            {
                b2Body* body = static_cast<b2Body*>(RuntimeBody);
                const auto& position = body->GetPosition();
                Transform.setPosition(position.x, position.y);
            }
            return Transform.getPosition();
        }

        void setRotation(float angle) {
            if (RuntimeBody)
            {
                b2Body* body = static_cast<b2Body*>(RuntimeBody);
                body->SetTransform(b2Vec2(getPosition().x, getPosition().y), DEG_TO_RAD(angle));
            }
            Transform.setRotation(angle);
        }

        float getRotation() {
            if (RuntimeBody)
            {
                b2Body* body = static_cast<b2Body*>(RuntimeBody);
                Transform.setRotation(RAD_TO_DEG(body->GetAngle()));
            }
            return Transform.getRotation();
        }

        void setScale(float scaleX, float scaleY) {
            Transform.setScale(scaleX, scaleY);
        }

        sf::Vector2f getScale() const {
            return Transform.getScale();
        }

        void setOrigin(float x, float y) {
            Transform.setOrigin(x, y);
        }

        sf::Vector2f getOrigin() const {
            return Transform.getOrigin();
        }

        void move(float offsetX, float offsetY) {
            Transform.move(offsetX, offsetY);
        }

        void rotate(float angle) {
            Transform.rotate(angle);
        }

        void scale(float factorX, float factorY) {
            Transform.scale(factorX, factorY);
        }

        sf::Transform getTransform() const {
            return Transform.getTransform();
        }

        void reset() {
            Transform = sf::Transformable();
        }
    };
}
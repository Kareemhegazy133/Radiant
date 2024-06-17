#pragma once

#include "SFML/Graphics/Transformable.hpp"

#include "World/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class TransformComponent : public Component
    {
    public:
        sf::Transformable Transform;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const sf::Vector2f& position) : Transform()
        {
            setPosition(position);
        }

        operator const sf::Transformable& () { return Transform; }

        void setPosition(const sf::Vector2f& position) {
            Transform.setPosition(position);
        }

        void setPosition(float x, float y) {
            Transform.setPosition(x, y);
        }

        sf::Vector2f getPosition() const {
            return Transform.getPosition();
        }

        void setRotation(float angle) {
            Transform.setRotation(angle);
        }

        float getRotation() const {
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
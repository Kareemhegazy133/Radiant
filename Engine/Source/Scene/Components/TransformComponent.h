#pragma once

#include "SFML/Graphics/Transformable.hpp"

#include "Scene/Components/Component.h"

namespace Engine {

    class TransformComponent : public Component
    {
    public:

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const sf::Vector2f& position);

        void SetPosition(const sf::Vector2f& position);
        void SetPosition(float x, float y);
        sf::Vector2f GetPosition();

        void SetRotation(float angle);
        float GetRotation();

        void SetScale(float scaleX, float scaleY);
        sf::Vector2f GetScale() const;

        void SetOrigin(float x, float y);
        sf::Vector2f GetOrigin() const;

        void Move(float offsetX, float offsetY);

        void Rotate(float angle);

        void Scale(float factorX, float factorY);

        sf::Transform GetTransform() const;

        void Reset();

        operator const sf::Transformable& () { return Transform; }
        operator sf::Transformable& () { return Transform; }

    public:
        // Storage for runtime b2Body
        void* RuntimeBody = nullptr;

    private:
        sf::Transformable Transform;
    };
}
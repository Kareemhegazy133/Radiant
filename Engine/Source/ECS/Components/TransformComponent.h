#pragma once

#include <SFML/Graphics/Transformable.hpp>

namespace Engine {

    class TransformComponent
    {
    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec2& position);

        void SetPosition(const glm::vec2& position);
        void SetPosition(float x, float y);
        glm::vec2 GetPosition();

        void SetRotation(float angle);
        float GetRotation();

        void SetScale(const glm::vec2& scale);
        void SetScale(float scaleX, float scaleY);
        glm::vec2 GetScale() const;

        void SetOrigin(float x, float y);
        glm::vec2 GetOrigin() const;

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
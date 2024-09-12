#pragma once

#include <SFML/Graphics.hpp>
#include <box2d/b2_draw.h>

#include "Core/GameApplication.h"

namespace Engine {

    class DebugDraw : public b2Draw
    {
    public:
        DebugDraw()
            : m_RenderWindow(GameApplication::GetRenderWindow()) {}

        // Override virtual methods from b2Draw
        void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
        void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
        void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;
        void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override;
        void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;
        void DrawTransform(const b2Transform& xf) override;
        void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

    private:
        sf::RenderWindow* m_RenderWindow;

        sf::Color B2ColorToSFColor(const b2Color& color, int alpha = 255);

        // Conversion functions between GLM and SFML
        sf::Vector2f GLMVecToSFVec(const glm::vec2& vector);
        sf::Vector2f GLMVecToSFVec(const glm::vec3& vector);
        sf::Vector2f GLMVecToSFVec(const glm::vec4& vector);
        glm::vec2 SFVecToGLMVec(const sf::Vector2f& vector);
    };
}


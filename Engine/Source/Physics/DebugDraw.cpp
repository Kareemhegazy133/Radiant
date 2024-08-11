#include "Enginepch.h"

#include "DebugDraw.h"

namespace Engine {

    sf::Color DebugDraw::B2ColorToSFColor(const b2Color& color, int alpha)
    {
        return sf::Color(static_cast<sf::Uint8>(color.r * 255),
            static_cast<sf::Uint8>(color.g * 255),
            static_cast<sf::Uint8>(color.b * 255),
            static_cast<sf::Uint8>(alpha));
    }

    sf::Vector2f DebugDraw::B2VecToSFVec(const b2Vec2& vector)
    {
        return sf::Vector2f(vector.x, vector.y);
    }

    void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
    {
        sf::ConvexShape polygon(vertexCount);
        for (int i = 0; i < vertexCount; i++)
        {
            polygon.setPoint(i, B2VecToSFVec(vertices[i]));
        }
        polygon.setOutlineThickness(-1.0f);
        polygon.setOutlineColor(B2ColorToSFColor(color));
        polygon.setFillColor(sf::Color::Transparent);

        m_RenderWindow->draw(polygon);
    }

    void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
    {
        sf::ConvexShape polygon(vertexCount);
        for (int i = 0; i < vertexCount; i++)
        {
            polygon.setPoint(i, B2VecToSFVec(vertices[i]));
        }
        polygon.setOutlineThickness(-1.0f);
        polygon.setOutlineColor(B2ColorToSFColor(color));
        polygon.setFillColor(B2ColorToSFColor(color, 60));  // Semi-transparent fill

        m_RenderWindow->draw(polygon);
    }

    void DebugDraw::DrawCircle(const b2Vec2& center, float radius, const b2Color& color)
    {
        sf::CircleShape circle(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(B2VecToSFVec(center));
        circle.setOutlineThickness(-1.0f);
        circle.setOutlineColor(B2ColorToSFColor(color));
        circle.setFillColor(sf::Color::Transparent);

        m_RenderWindow->draw(circle);
    }

    void DebugDraw::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color)
    {
        sf::CircleShape circle(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(B2VecToSFVec(center));
        circle.setOutlineThickness(-1.0f);
        circle.setOutlineColor(B2ColorToSFColor(color));
        circle.setFillColor(B2ColorToSFColor(color, 60));  // Semi-transparent fill

        // Draw the line showing the axis
        sf::VertexArray line(sf::Lines, 2);
        line[0].position = B2VecToSFVec(center);
        line[0].color = B2ColorToSFColor(color);
        line[1].position = B2VecToSFVec(center + radius * axis);
        line[1].color = B2ColorToSFColor(color);

        m_RenderWindow->draw(circle);
        m_RenderWindow->draw(line);
    }

    void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
    {
        sf::VertexArray line(sf::Lines, 2);
        line[0].position = B2VecToSFVec(p1);
        line[0].color = B2ColorToSFColor(color);
        line[1].position = B2VecToSFVec(p2);
        line[1].color = B2ColorToSFColor(color);

        m_RenderWindow->draw(line);
    }

    void DebugDraw::DrawTransform(const b2Transform& xf)
    {
        const float lineLength = 0.4f;

        b2Vec2 p1 = xf.p, p2;
        sf::VertexArray line(sf::Lines, 2);

        // Red x-axis
        line[0].position = B2VecToSFVec(p1);
        line[0].color = sf::Color::Red;
        p2 = p1 + lineLength * xf.q.GetXAxis();
        line[1].position = B2VecToSFVec(p2);
        line[1].color = sf::Color::Red;
        m_RenderWindow->draw(line);

        // Green y-axis
        line[0].position = B2VecToSFVec(p1);
        line[0].color = sf::Color::Green;
        p2 = p1 + lineLength * xf.q.GetYAxis();
        line[1].position = B2VecToSFVec(p2);
        line[1].color = sf::Color::Green;
        m_RenderWindow->draw(line);
    }

    void DebugDraw::DrawPoint(const b2Vec2& p, float size, const b2Color& color)
    {
        sf::CircleShape point(size);
        point.setOrigin(size, size);
        point.setPosition(B2VecToSFVec(p));
        point.setFillColor(B2ColorToSFColor(color));

        m_RenderWindow->draw(point);
    }
}
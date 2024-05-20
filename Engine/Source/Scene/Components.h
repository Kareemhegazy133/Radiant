#pragma once

#include "SFML/Graphics/Sprite.hpp"

namespace Engine {

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };

    struct TransformComponent
    {
        sf::Transformable Transform;

        TransformComponent() = default;

        void setPosition(float x, float y) {
            Transform.setPosition(x, y);
        }

        void setRotation(float angle) {
            Transform.setRotation(angle);
        }

        void setScale(float scaleX, float scaleY) {
            Transform.setScale(scaleX, scaleY);
        }

        void reset() {
            Transform = sf::Transformable();
        }
    };

	struct SpriteComponent
	{
		sf::Sprite Sprite;

		SpriteComponent() = default;

		SpriteComponent(const sf::Texture& texture) : Sprite(texture) {}

		SpriteComponent(const sf::Texture& texture, const sf::IntRect& rectangle) : Sprite(texture, rectangle) {}

		void setTexture(const sf::Texture& texture) {
            Sprite.setTexture(texture);
		}
	};
}
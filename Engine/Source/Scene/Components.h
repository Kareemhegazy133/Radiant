#pragma once

#include <SFML/System/String.hpp>
#include "SFML/Graphics/Sprite.hpp"

namespace Engine {

    struct TagComponent
    {
        sf::String Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const sf::String& tag) : Tag(tag) {}
    };

    struct TransformComponent
    {
        sf::Transform Transform;

        TransformComponent() : Transform() {}

        void setPosition(float x, float y) {
            Transform.translate(x, y);
        }

        void setRotation(float angle) {
            Transform.rotate(angle);
        }

        void setScale(float scaleX, float scaleY) {
            Transform.scale(scaleX, scaleY);
        }

        void reset() {
            Transform = sf::Transform::Identity;
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
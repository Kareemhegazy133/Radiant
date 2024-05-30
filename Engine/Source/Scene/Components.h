#pragma once

#include "SFML/Graphics/Sprite.hpp"
#include "Utils/TextureManager.h"

namespace Engine {

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}

        operator const std::string& () { return Tag; }
    };

    struct TransformComponent
    {
        sf::Transformable Transform;

        TransformComponent() = default;
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

    struct AnimationComponent
    {
        struct Animation
        {
            std::vector<sf::IntRect> Frames;
            float FrameDuration;

            Animation() = default;

            Animation(const std::vector<sf::IntRect>& frames, float frameDuration)
                : Frames(frames), FrameDuration(frameDuration)
            {}
        };

        std::unordered_map<std::string, Animation> Animations;
        std::string CurrentAnimation;
        float ElapsedTime;
        unsigned int CurrentFrame;

        AnimationComponent() : ElapsedTime(0.0f), CurrentFrame(0) {}

        // Add an animation
        void AddAnimation(const std::string& name, const std::vector<sf::IntRect>& frames, float frameDuration) {
            Animations[name] = Animation(frames, frameDuration);
        }

        // Set the current animation
        void SetAnimation(const std::string& name) {
            if (Animations.find(name) != Animations.end()) {
                CurrentAnimation = name;
                CurrentFrame = 0;
                ElapsedTime = 0.0f;
            }
            else {
                // Handle the error case where the animation name is not found
                ENGINE_ASSERT("Animation named: {0} is not found!", name);
            }
        }

        // Update the animation frame based on the elapsed time
        void Update(Timestep ts) {
            if (CurrentAnimation.empty()) return;

            ElapsedTime += ts;
            const Animation& animation = Animations[CurrentAnimation];

            if (ElapsedTime >= animation.FrameDuration) {
                ElapsedTime = 0.0f;
                // Move to next frame, loop back if at the end
                CurrentFrame = (CurrentFrame + 1) % animation.Frames.size();
            }
        }

        // Get the current frame rectangle
        const sf::IntRect& GetCurrentFrame() const {
            return Animations.at(CurrentAnimation).Frames[CurrentFrame];
        }
    };

	struct SpriteComponent
	{
		sf::Sprite Sprite;
        std::string TextureIdentifier; // Identifier for the texture in the TextureManager
        AnimationComponent* Animation; // Pointer to an AnimationComponent (optional)

        SpriteComponent() : Animation(nullptr) {}

        SpriteComponent(const std::string& textureIdentifier, AnimationComponent* animation = nullptr)
            : Sprite(), TextureIdentifier(textureIdentifier), Animation(animation)
        {
            // Set the texture using the TextureManager
            Sprite.setTexture(TextureManager::Get().getTexture(textureIdentifier));

            // If there's an animation, set the initial texture rectangle
            if (Animation) {
                Sprite.setTextureRect(Animation->GetCurrentFrame());
            }
        }

        operator const sf::Sprite& () { return Sprite; }

        // Function to set the texture identifier and update the texture of the sprite
        void setTextureIdentifier(const std::string& textureIdentifier) {
            TextureIdentifier = textureIdentifier;
            Sprite.setTexture(TextureManager::Get().getTexture(textureIdentifier));
        }

        // Function to get the texture identifier
        const std::string& getTextureIdentifier() const {
            return TextureIdentifier;
        }

        void setTextureRect(const sf::IntRect& rectangle) {
            Sprite.setTextureRect(rectangle);
        }

        void setPosition(const sf::Vector2f& position) {
            Sprite.setPosition(position);
        }

        void setPosition(float x, float y) {
            Sprite.setPosition(x, y);
        }

        sf::Vector2f getPosition() const {
            return Sprite.getPosition();
        }

        void setRotation(float angle) {
            Sprite.setRotation(angle);
        }

        float getRotation() const {
            return Sprite.getRotation();
        }

        void setScale(const sf::Vector2f& scale) {
            Sprite.setScale(scale);
        }

        void setScale(float factorX, float factorY) {
            Sprite.setScale(factorX, factorY);
        }

        sf::Vector2f getScale() const {
            return Sprite.getScale();
        }

        void setOrigin(float x, float y) {
            Sprite.setOrigin(x, y);
        }

        void setColor(const sf::Color& color) {
            Sprite.setColor(color);
        }

        // Update the sprite's animation
        void update(Timestep ts) {
            if (Animation) {
                Animation->Update(ts);
                Sprite.setTextureRect(Animation->GetCurrentFrame());
            }
        }
	};

}
#pragma once

#include "SFML/Graphics/Rect.hpp"

#include "Core/Timestep.h"
#include "World/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class AnimationComponent : public Component
    {
    public:
        struct Animation
        {
            std::vector<sf::IntRect> Frames;
            float FrameDuration;
            bool Loop = true;
            uint8_t FrameWidthPadding;
            uint8_t FrameHeightPadding;

            Animation() = default;

            Animation(const std::vector<sf::IntRect>& frames, float frameDuration, bool enableLooping = true)
                : Frames(frames), FrameDuration(frameDuration), Loop(enableLooping)
            {}
        };

        std::unordered_map<std::string, Animation> Animations;
        std::string CurrentAnimation;
        float ElapsedTime;
        unsigned int CurrentFrame;

        AnimationComponent() : ElapsedTime(0.0f), CurrentFrame(0) {}

        // Add an animation
        void AddAnimation(const std::string& name, const std::vector<sf::IntRect>& frames, float frameDuration, bool enableLooping) {
            Animations[name] = Animation(frames, frameDuration, enableLooping);
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

        // Get the current size of the frame rectangle (Ignores Padding)
        sf::Vector2i GetCurrentFrameSize() const {
            const auto& currentAnimation = Animations.at(CurrentAnimation);
            const auto& currentFrame = currentAnimation.Frames.at(CurrentFrame);

            sf::Vector2i size = {
                currentFrame.getSize().x - currentAnimation.FrameWidthPadding * 2,
                currentFrame.getSize().y - currentAnimation.FrameHeightPadding
            };
            return size;
        }
    };
}
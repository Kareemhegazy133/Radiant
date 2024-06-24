#pragma once

#include "SFML/Graphics/Rect.hpp"

#include "Core/Timestep.h"
#include "World/Components/Component.h"

#include "Enginepch.h"

namespace Engine {

    class AnimationComponent : public Component
    {
    public:

        AnimationComponent();

        void AddAnimation(const std::string& name, const std::vector<sf::IntRect>& frames, float frameDuration, bool enableLooping);

        void SetAnimation(const std::string& name);

        // Update the animation frame based on the elapsed time
        void Update(Timestep ts);

        const sf::IntRect& GetCurrentFrame() const;

        // Get the current size of the frame rectangle (Ignores Padding)
        sf::Vector2i GetCurrentFrameSize() const;

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
    };
}
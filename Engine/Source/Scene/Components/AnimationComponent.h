#pragma once

#include "SFML/Graphics/Rect.hpp"

#include "Core/Timestep.h"
#include "Scene/Components/Component.h"
#include "Scene/Components/SpriteComponent.h"

#include "Enginepch.h"

namespace Engine {

    class AnimationComponent : public Component
    {
    public:

        AnimationComponent();

        void AddAnimation(const std::string& name,
            const std::vector<sf::IntRect>& frames,
            uint8_t frameWidthPadding,
            uint8_t frameHeightPadding,
            float frameDuration,
            bool enableLooping
        );

        void SetAnimation(const std::string& name);

        // Update the animation frame based on the elapsed time
        void Update(Timestep ts);

        const sf::IntRect& GetCurrentFrame() const;

    public:
        struct Animation
        {
            std::vector<sf::IntRect> Frames;
            float FrameDuration;
            bool Loop = true;
            uint8_t FrameWidthPadding = 0;
            uint8_t FrameHeightPadding = 0;

            Animation() = default;

            Animation(const std::vector<sf::IntRect>& frames,
                float frameDuration,
                uint8_t frameWidthPadding = 0,
                uint8_t frameHeightPadding = 0,
                bool enableLooping = true
            )
                : Frames(frames),
                FrameDuration(frameDuration),
                FrameWidthPadding(frameWidthPadding),
                FrameHeightPadding(frameHeightPadding),
                Loop(enableLooping)
            {}
        };

        std::unordered_map<std::string, Animation> Animations;
        SpriteComponent* Sprite = nullptr;
    private:
        std::string m_CurrentAnimation;
        float m_ElapsedTime;
        unsigned int m_CurrentFrame;
    };
}
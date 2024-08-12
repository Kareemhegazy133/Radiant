#pragma once

#include "SFML/Graphics/Rect.hpp"

#include "Core/Timestep.h"
#include "Scene/Components/Component.h"
#include "Scene/Components/SpriteComponent.h"

#include "Enginepch.h"

enum class CharacterState;
enum class AbilityState;

namespace Engine {

    class AnimationComponent : public Component
    {
    public:
        AnimationComponent();

        template<typename T>
        void AddAnimation(T state,
            const std::string& textureIdentifier,
            const std::vector<sf::IntRect>& frames,
            uint8_t frameWidthPadding,
            uint8_t frameHeightPadding,
            float frameDuration,
            bool enableLooping
        );

        template<typename T>
        void SetAnimation(T state);

        // Update the animation frame based on the elapsed time
        void Update(Timestep ts);

        const sf::IntRect& GetCurrentFrame() const;

    public:
        struct Animation
        {
            std::string TextureIdentifier;
            std::vector<sf::IntRect> Frames;
            float FrameDuration;
            bool Loop = true;
            uint8_t FrameWidthPadding = 0;
            uint8_t FrameHeightPadding = 0;

            Animation() = default;

            Animation(const std::string& textureIdentifier, const std::vector<sf::IntRect>& frames,
                float frameDuration,
                uint8_t frameWidthPadding = 0,
                uint8_t frameHeightPadding = 0,
                bool enableLooping = true
            )
                : TextureIdentifier(textureIdentifier),
                Frames(frames),
                FrameDuration(frameDuration),
                FrameWidthPadding(frameWidthPadding),
                FrameHeightPadding(frameHeightPadding),
                Loop(enableLooping)
            {}
        };

        std::unordered_map<int, Animation> Animations;
        SpriteComponent* Sprite = nullptr;
    private:
        int m_CurrentAnimation = -1;
        float m_ElapsedTime;
        unsigned int m_CurrentFrame;
    };
}
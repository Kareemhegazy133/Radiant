#include "Enginepch.h"

#include "AnimationComponent.h"

namespace Engine {

    AnimationComponent::AnimationComponent()
        : m_ElapsedTime(0.0f), m_CurrentFrame(0)
    {
    }

    template<typename T>
    void AnimationComponent::AddAnimation(T state,
        const std::string& textureIdentifier,
        const std::vector<sf::IntRect>& frames,
        uint8_t frameWidthPadding,
        uint8_t frameHeightPadding,
        float frameDuration,
        bool enableLooping
    )
    {
        ENGINE_ASSERT(std::is_enum<T>::value, "T must be an enum type");

        int stateKey = static_cast<int>(state);
        Animations[stateKey] = Animation(textureIdentifier, frames, frameDuration, frameWidthPadding, frameHeightPadding, enableLooping);
    }

    template<typename T>
    void AnimationComponent::SetAnimation(T state)
    {
        ENGINE_ASSERT(std::is_enum<T>::value, "T must be an enum type");

        int stateKey = static_cast<int>(state);
        if (Animations.find(stateKey) != Animations.end())
        {
            // Only reset if the animation is different or if it's a non-looping animation
            if (m_CurrentAnimation != stateKey || !Animations[stateKey].Loop)
            {
                m_CurrentAnimation = stateKey;
                m_CurrentFrame = 0;
                m_ElapsedTime = 0.0f;
                Sprite->SetTextureIdentifier(Animations[m_CurrentAnimation].TextureIdentifier);
            }
        }
        else {
            ENGINE_ASSERT("Animation state not found!");
        }
    }

    void AnimationComponent::Update(Timestep ts)
    {
        // No animation
        if (m_CurrentAnimation == -1) return;

        m_ElapsedTime += ts;
        const Animation& animation = Animations[m_CurrentAnimation];

        if (m_ElapsedTime >= animation.FrameDuration)
        {
            m_ElapsedTime = 0.0f;

            if (animation.Loop)
            {
                // Move to next frame, loop back if at the end
                m_CurrentFrame = (m_CurrentFrame + 1) % animation.Frames.size();
            }
            else
            {
                // Move to next frame, but stop at the last frame
                if (m_CurrentFrame < animation.Frames.size() - 1)
                {
                    m_CurrentFrame++;
                }
            }
        }

        Sprite->SetTextureRect(GetCurrentFrame());
    }

    void AnimationComponent::ResetAnimation()
    {
        m_CurrentFrame = 0;
        m_ElapsedTime = 0.0f;
        if (m_CurrentAnimation != -1)
        {
            Sprite->SetTextureIdentifier(Animations[m_CurrentAnimation].TextureIdentifier);
        }
    }


    bool AnimationComponent::IsFinished() const
    {
        // No animation or looping animation cannot be finished
        if (m_CurrentAnimation == -1 || Animations.at(m_CurrentAnimation).Loop)
            return false;

        // If we are on the last frame and the animation is not looping
        return m_CurrentFrame == Animations.at(m_CurrentAnimation).Frames.size() - 1;
    }

    const sf::IntRect& AnimationComponent::GetCurrentFrame() const
    {
        return Animations.at(m_CurrentAnimation).Frames[m_CurrentFrame];
    }

    // Explicit template instantiation
    template void AnimationComponent::AddAnimation<PlayerState>(
        PlayerState,
        const std::string&,
        const std::vector<sf::IntRect>&,
        uint8_t,
        uint8_t,
        float,
        bool
    );

    template void AnimationComponent::AddAnimation<AbilityState>(
        AbilityState,
        const std::string&,
        const std::vector<sf::IntRect>&,
        uint8_t,
        uint8_t,
        float,
        bool
    );

    template void AnimationComponent::SetAnimation<PlayerState>(PlayerState);
    template void AnimationComponent::SetAnimation<AbilityState>(AbilityState);
}

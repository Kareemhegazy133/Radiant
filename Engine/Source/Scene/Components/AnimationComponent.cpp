#include "Enginepch.h"

#include "AnimationComponent.h"

namespace Engine {

    AnimationComponent::AnimationComponent()
        : m_ElapsedTime(0.0f), m_CurrentFrame(0)
    {
    }

    void AnimationComponent::AddAnimation(const std::string& name,
        const std::vector<sf::IntRect>& frames,
        uint8_t frameWidthPadding,
        uint8_t frameHeightPadding,
        float frameDuration,
        bool enableLooping
    )
    {
        Animations[name] = Animation(frames, frameDuration, frameWidthPadding, frameHeightPadding, enableLooping);
    }

    void AnimationComponent::SetAnimation(const std::string& name)
    {
        if (Animations.find(name) != Animations.end())
        {
            m_CurrentAnimation = name;
            m_CurrentFrame = 0;
            m_ElapsedTime = 0.0f;
            Sprite->SetTextureIdentifier(name);
        }
        else
        {
            // Handle the error case where the animation name is not found
            ENGINE_ASSERT("Animation named: {0} is not found!", name);
        }
    }

    void AnimationComponent::Update(Timestep ts)
    {
        if (m_CurrentAnimation.empty()) return;

        m_ElapsedTime += ts;
        const Animation& animation = Animations[m_CurrentAnimation];

        if (m_ElapsedTime >= animation.FrameDuration)
        {
            m_ElapsedTime = 0.0f;
            // Move to next frame, loop back if at the end
            m_CurrentFrame = (m_CurrentFrame + 1) % animation.Frames.size();
        }

        Sprite->SetTextureRect(GetCurrentFrame());
    }

    const sf::IntRect& AnimationComponent::GetCurrentFrame() const
    {
        return Animations.at(m_CurrentAnimation).Frames[m_CurrentFrame];
    }
}

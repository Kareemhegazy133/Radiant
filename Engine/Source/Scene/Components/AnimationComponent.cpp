#include "Enginepch.h"

#include "AnimationComponent.h"

namespace Engine {

    AnimationComponent::AnimationComponent()
        : ElapsedTime(0.0f), CurrentFrame(0)
    {
    }

    void AnimationComponent::AddAnimation(const std::string& name, const std::vector<sf::IntRect>& frames, float frameDuration, bool enableLooping)
    {
        Animations[name] = Animation(frames, frameDuration, enableLooping);
    }

    void AnimationComponent::SetAnimation(const std::string& name)
    {
        if (Animations.find(name) != Animations.end())
        {
            CurrentAnimation = name;
            CurrentFrame = 0;
            ElapsedTime = 0.0f;
        }
        else
        {
            // Handle the error case where the animation name is not found
            ENGINE_ASSERT("Animation named: {0} is not found!", name);
        }
    }

    void AnimationComponent::Update(Timestep ts)
    {
        if (CurrentAnimation.empty()) return;

        ElapsedTime += ts;
        const Animation& animation = Animations[CurrentAnimation];

        if (ElapsedTime >= animation.FrameDuration)
        {
            ElapsedTime = 0.0f;
            // Move to next frame, loop back if at the end
            CurrentFrame = (CurrentFrame + 1) % animation.Frames.size();
        }
    }

    const sf::IntRect& AnimationComponent::GetCurrentFrame() const
    {
        return Animations.at(CurrentAnimation).Frames[CurrentFrame];
    }

    sf::Vector2i AnimationComponent::GetCurrentFrameSize() const
    {
        const auto& currentAnimation = Animations.at(CurrentAnimation);
        const auto& currentFrame = currentAnimation.Frames.at(CurrentFrame);

        sf::Vector2i size = {
            currentFrame.getSize().x - currentAnimation.FrameWidthPadding * 2,
            currentFrame.getSize().y - currentAnimation.FrameHeightPadding
        };
        return size;
    }
}

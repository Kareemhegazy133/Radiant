#pragma once

#include "Entity.h"

enum class PlayerState;
enum class AbilityState;
enum class EnemyState;

namespace Engine {

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() {}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		bool operator==(const ScriptableEntity& other) const
		{
			return m_Entity == other.m_Entity;
		}

		bool operator!=(const ScriptableEntity& other) const
		{
			return !(*this == other);
		}

		bool operator==(const Entity& other) const
		{
			return m_Entity == other;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	protected:
		virtual void OnCreate() { std::cout << "Scriptable OnCreate" << std::endl; }
		virtual void OnUpdate(Timestep ts) { std::cout << "Scriptable OnUpdate" << std::endl; }
		virtual void OnDestroy() { std::cout << "Scriptable OnDestroy" << std::endl; }

		template<typename T>
		void SetupAnimation(T state, const std::string& textureIdentifier, int frameCount, int frameWidth, int frameHeight,
			int frameWidthPadding, int frameHeightPadding, float frameDuration, bool enableLooping)
		{
			std::vector<sf::IntRect> frames;
			for (int i = 0; i < frameCount; i++)
			{
				int left = frameWidth * i + frameWidthPadding;
				int top = frameHeightPadding;
				int width = frameWidth - frameWidthPadding * 2;
				int height = frameHeight - frameHeightPadding * 2;
				frames.emplace_back(sf::IntRect(left, top, width, height));
			}

			auto& animationComponent = GetComponent<AnimationComponent>();
			animationComponent.AddAnimation(state, textureIdentifier, frames, frameWidthPadding, frameHeightPadding, frameDuration, enableLooping);
		}

	protected:
		uint16_t m_FrameWidth;
		uint16_t m_FrameHeight;
		uint8_t m_FrameWidthPadding = 0;
		uint8_t m_FrameHeightPadding = 0;

	private:
		Entity m_Entity;
		friend class Level;
	};

	//// Explicit template instantiation
	//template void ScriptableEntity::SetupAnimation<PlayerState>(PlayerState, const std::string&, int, int, int, int, int, float, bool);
	//template void ScriptableEntity::SetupAnimation<AbilityState>(AbilityState, const std::string&, int, int, int, int, int, float, bool);
	//template void ScriptableEntity::SetupAnimation<EnemyState>(EnemyState, const std::string&, int, int, int, int, int, float, bool);
}
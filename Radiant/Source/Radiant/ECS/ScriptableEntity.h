#pragma once

#include "Entity.h"

namespace Radiant {

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() = default;

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
		virtual void OnCreate() { RADIANT_INFO("Scriptable OnCreate"); }
		virtual void OnUpdate(Timestep ts) { RADIANT_INFO("Scriptable OnUpdate"); }
		virtual void OnDestroy() { RADIANT_INFO("Scriptable OnDestroy"); }

	//	template<typename T>
	//	void SetupAnimation(T state, const std::string& textureIdentifier, int frameCount, int frameWidth, int frameHeight,
	//		int frameWidthPadding, int frameHeightPadding, float frameDuration, bool enableLooping)
	//	{
	//		std::vector<glm::ivec4> frames;
	//		for (int i = 0; i < frameCount; i++)
	//		{
	//			int left = frameWidth * i + frameWidthPadding;
	//			int top = frameHeightPadding;
	//			int width = frameWidth - frameWidthPadding * 2;
	//			int height = frameHeight - frameHeightPadding * 2;
	//			frames.emplace_back(left, top, width, height);
	//		}

	//		auto& animationComponent = GetComponent<AnimationComponent>();
	//		animationComponent.AddAnimation(state, textureIdentifier, frames, frameWidthPadding, frameHeightPadding, frameDuration, enableLooping);
	//	}

	//protected:
	//	uint16_t m_FrameWidth;
	//	uint16_t m_FrameHeight;
	//	uint8_t m_FrameWidthPadding = 0;
	//	uint8_t m_FrameHeightPadding = 0;

	private:
		Entity m_Entity;
		friend class Level;
	};

}
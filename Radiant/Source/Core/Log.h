#pragma once

#include "Core/Base.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Radiant {

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetRadiantLogger() { return s_RadiantLogger; }
		static Ref<spdlog::logger>& GetGameLogger() { return s_GameLogger; }
	private:
		static Ref<spdlog::logger> s_RadiantLogger;
		static Ref<spdlog::logger> s_GameLogger;
	};

}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

// Core log macros
#define RADIANT_TRACE(...)    ::Radiant::Log::GetRadiantLogger()->trace(__VA_ARGS__)
#define RADIANT_INFO(...)     ::Radiant::Log::GetRadiantLogger()->info(__VA_ARGS__)
#define RADIANT_WARN(...)     ::Radiant::Log::GetRadiantLogger()->warn(__VA_ARGS__)
#define RADIANT_ERROR(...)    ::Radiant::Log::GetRadiantLogger()->error(__VA_ARGS__)
#define RADIANT_CRITICAL(...) ::Radiant::Log::GetRadiantLogger()->critical(__VA_ARGS__)

// Client log macros
#define GAME_TRACE(...)         ::Radiant::Log::GetGameLogger()->trace(__VA_ARGS__)
#define GAME_INFO(...)          ::Radiant::Log::GetGameLogger()->info(__VA_ARGS__)
#define GAME_WARN(...)          ::Radiant::Log::GetGameLogger()->warn(__VA_ARGS__)
#define GAME_ERROR(...)         ::Radiant::Log::GetGameLogger()->error(__VA_ARGS__)
#define GAME_CRITICAL(...)      ::Radiant::Log::GetGameLogger()->critical(__VA_ARGS__)
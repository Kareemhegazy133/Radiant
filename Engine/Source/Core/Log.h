#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Engine {

	class Log
	{
		public:
			static void Init();

			inline static std::shared_ptr<spdlog::logger>& GetEngineLogger() { return s_EngineLogger; }
			inline static std::shared_ptr<spdlog::logger>& GetGameLogger() { return s_GameLogger; }
		private:
			static std::shared_ptr<spdlog::logger> s_EngineLogger;
			static std::shared_ptr<spdlog::logger> s_GameLogger;
	};

}

// Engine log macros
#define ENGINE_TRACE(...)   ::Engine::Log::GetEngineLogger()->trace(__VA_ARGS__)
#define ENGINE_INFO(...)    ::Engine::Log::GetEngineLogger()->info(__VA_ARGS__)
#define ENGINE_WARN(...)    ::Engine::Log::GetEngineLogger()->warn(__VA_ARGS__)
#define ENGINE_ERROR(...)   ::Engine::Log::GetEngineLogger()->error(__VA_ARGS__)
#define ENGINE_FATAL(...)   ::Engine::Log::GetEngineLogger()->critical(__VA_ARGS__)

// Game log macros
#define GAME_TRACE(...)        ::Engine::Log::GetGameLogger()->trace(__VA_ARGS__)
#define GAME_INFO(...)         ::Engine::Log::GetGameLogger()->info(__VA_ARGS__)
#define GAME_WARN(...)         ::Engine::Log::GetGameLogger()->warn(__VA_ARGS__)
#define GAME_ERROR(...)        ::Engine::Log::GetGameLogger()->error(__VA_ARGS__)
#define GAME_FATAL(...)        ::Engine::Log::GetGameLogger()->critical(__VA_ARGS__)
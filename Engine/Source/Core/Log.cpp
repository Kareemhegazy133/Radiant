#include "Enginepch.h"
#include "Core/Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Engine {

	std::shared_ptr<spdlog::logger> Log::s_EngineLogger;
	std::shared_ptr<spdlog::logger> Log::s_GameLogger;

	void Log::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_EngineLogger = spdlog::stdout_color_mt("ENGINE");
		s_EngineLogger->set_level(spdlog::level::trace);

		s_GameLogger = spdlog::stdout_color_mt("GAME");
		s_GameLogger->set_level(spdlog::level::trace);
	}
}
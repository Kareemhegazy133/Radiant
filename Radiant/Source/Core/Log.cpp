#include "rdpch.h"
#include "Core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Radiant {

	Ref<spdlog::logger> Log::s_RadiantLogger;
	Ref<spdlog::logger> Log::s_GameLogger;

	void Log::Init()
	{
		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Radiant.log", true));

		logSinks[0]->set_pattern("%^[%T] %n: %v%$");
		logSinks[1]->set_pattern("[%T] [%l] %n: %v");

		s_RadiantLogger = std::make_shared<spdlog::logger>("RADIANT", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_RadiantLogger);
		s_RadiantLogger->set_level(spdlog::level::trace);
		s_RadiantLogger->flush_on(spdlog::level::trace);

		s_GameLogger = std::make_shared<spdlog::logger>("GAME", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_GameLogger);
		s_GameLogger->set_level(spdlog::level::trace);
		s_GameLogger->flush_on(spdlog::level::trace);
	}

}

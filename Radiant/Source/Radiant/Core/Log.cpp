#include "Radiant/rdpch.h"
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Radiant {

	std::shared_ptr<spdlog::logger> Log::s_RadiantLogger;
	std::shared_ptr<spdlog::logger> Log::s_GameLogger;

	void Log::Init()
	{
		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		// [%L] carries the level even where color doesn't (colorblind readers,
		// CI captures, piped output); the file pattern spells it out fully
		logSinks[0]->set_pattern("%^[%T] [%L] %n: %v%$");

		// A locked or unwritable Radiant.log must not kill the process before
		// logging exists — recover to console-only and report it below
		std::string fileSinkError;
		try
		{
			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Radiant.log", true);
			fileSink->set_pattern("[%T] [%l] %n: %v");
			logSinks.emplace_back(fileSink);
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			fileSinkError = ex.what();
		}

		// flush_on(trace): flush every message so a crash never swallows the log
		// tail — throughput traded for diagnosability, fine for iteration configs
		s_RadiantLogger = std::make_shared<spdlog::logger>("RADIANT", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_RadiantLogger);
		s_RadiantLogger->set_level(spdlog::level::trace);
		s_RadiantLogger->flush_on(spdlog::level::trace);

		s_GameLogger = std::make_shared<spdlog::logger>("GAME", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_GameLogger);
		s_GameLogger->set_level(spdlog::level::trace);
		s_GameLogger->flush_on(spdlog::level::trace);

		if (!fileSinkError.empty())
			RADIANT_WARN("Log: could not open Radiant.log ({}) - continuing console-only", fileSinkError);
	}

}
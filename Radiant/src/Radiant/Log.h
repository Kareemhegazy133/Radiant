#pragma once

#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"

namespace Radiant {

	class RADIANT_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define RD_CORE_TRACE(...)   ::Radiant::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define RD_CORE_INFO(...)    ::Radiant::Log::GetCoreLogger()->info(__VA_ARGS__)
#define RD_CORE_WARN(...)    ::Radiant::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define RD_CORE_ERROR(...)   ::Radiant::Log::GetCoreLogger()->error(__VA_ARGS__)
#define RD_CORE_FATAL(...)   ::Radiant::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define RD_TRACE(...)        ::Radiant::Log::GetClientLogger()->trace(__VA_ARGS__)
#define RD_INFO(...)         ::Radiant::Log::GetClientLogger()->info(__VA_ARGS__)
#define RD_WARN(...)         ::Radiant::Log::GetClientLogger()->warn(__VA_ARGS__)
#define RD_ERROR(...)        ::Radiant::Log::GetClientLogger()->error(__VA_ARGS__)
#define RD_FATAL(...)        ::Radiant::Log::GetClientLogger()->critical(__VA_ARGS__)

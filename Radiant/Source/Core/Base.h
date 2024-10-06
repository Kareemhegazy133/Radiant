#pragma once

#include <memory>

#include "Core/PlatformDetection.h"

#ifdef RADIANT_DEBUG
	#if defined(RADIANT_PLATFORM_WINDOWS)
		#define RADIANT_DEBUGBREAK() __debugbreak()
	#elif defined(RADIANT_PLATFORM_LINUX)
		#include <signal.h>
		#define RADIANT_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define RADIANT_ENABLE_ASSERTS
#else
	#define RADIANT_DEBUGBREAK()
#endif

#define RADIANT_EXPAND_MACRO(x) x
#define RADIANT_STRINGIFY_MACRO(x) #x

#define BIT(x) (1u << x)

#define RADIANT_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define BIND_MEMBER_FUNCTION(function, instance) std::bind(&function, instance, std::placeholders::_1)

#define DEG_TO_RAD(angleInDegrees) ((angleInDegrees) * 3.14159265f / 180.0f)
#define RAD_TO_DEG(angleInRadians) ((angleInRadians) * 180.0f / 3.14159265f)

namespace Radiant {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	using byte = uint8_t;
}

#include "Core/Log.h"
#include "Core/Assert.h"
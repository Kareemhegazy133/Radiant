#pragma once

#include <memory>

#include "Radiant/Core/PlatformDetection.h"

// Checks are active in Debug AND Release (the day-to-day iteration configs) and
// compile out only in Dist — the build a player receives. In Release without a
// debugger attached, __debugbreak raises an unhandled breakpoint exception:
// a loud, immediate crash at the violation site (fail-fast by design).
#if defined(RD_DEBUG) || defined(RD_RELEASE)
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
	#define RADIANT_DEBUGBREAK() ((void)0)
#endif

// Tracks live RefCounted instances for leak diagnostics. On in Debug builds;
// flip to 1 manually to hunt leaks in optimized builds.
#ifdef RD_DEBUG
	#define RADIANT_TRACK_REFERENCES 1
#else
	#define RADIANT_TRACK_REFERENCES 0
#endif

#define RADIANT_EXPAND_MACRO(x) x
#define RADIANT_STRINGIFY_MACRO(x) #x

#define BIT(x) (1u << x)

// Binds a member function of the enclosing class as an event handler. Captures
// `this` raw — the resulting callable must not outlive the object it was made in.
#define RADIANT_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define BIND_MEMBER_FUNCTION(function, instance) std::bind(&function, instance, std::placeholders::_1)

#define DEG_TO_RAD(angleInDegrees) ((angleInDegrees) * 3.14159265f / 180.0f)
#define RAD_TO_DEG(angleInRadians) ((angleInRadians) * 180.0f / 3.14159265f)

namespace Radiant {

	/**
	 * Unique-ownership half of the engine's ownership vocabulary: the single
	 * owner creates and deletes (window, graphics context, subsystem data).
	 * Shared engine resources use Ref<T> instead — never both on one type.
	 */
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	using byte = uint8_t;
}

// Included after the macros above: Ref.h depends on RADIANT_DEBUGBREAK and
// RADIANT_TRACK_REFERENCES being defined first (Ref.h ↔ Base.h include cycle).
#include "Radiant/Core/Ref.h"

#include "Radiant/Core/Log.h"
#include "Radiant/Core/Assert.h"
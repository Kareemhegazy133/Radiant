#pragma once

#include "Radiant/Core/Base.h"
#include "Radiant/Core/Log.h"
#include <filesystem>

#ifdef RADIANT_ENABLE_ASSERTS

	// Alternatively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	// do-while(0) makes the expansion a single statement, so the assert is safe
	// inside a braceless if/else (a bare { } block there is a dangling-else bug)
	#define RADIANT_INTERNAL_ASSERT_IMPL(type, check, msg, ...) do { if(!(check)) { RADIANT##type##ERROR(msg, __VA_ARGS__); RADIANT_DEBUGBREAK(); } } while(0)
	#define RADIANT_INTERNAL_ASSERT_WITH_MSG(type, check, ...) RADIANT_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define RADIANT_INTERNAL_ASSERT_NO_MSG(type, check) RADIANT_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", RADIANT_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define RADIANT_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define RADIANT_INTERNAL_ASSERT_GET_MACRO(...) RADIANT_EXPAND_MACRO( RADIANT_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, RADIANT_INTERNAL_ASSERT_WITH_MSG, RADIANT_INTERNAL_ASSERT_NO_MSG) )

	// Usage: RADIANT_ASSERT(condition) or RADIANT_ASSERT(condition, "message").
	// The message must be a plain string literal — the 2-slot dispatcher below
	// cannot take format arguments (a third argument lands in the macro slot
	// and the expansion breaks). Pre-format if you need values.
	// For programmer errors only — config/content mistakes get a WARN and recovery.
	// Active in Debug and Release, compiled out entirely in Dist: never put side
	// effects in the condition, and never pass a message alone (a string literal
	// is always truthy, so it would assert nothing).
	#define RADIANT_ASSERT(...) RADIANT_EXPAND_MACRO( RADIANT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#else
	// ((void)0) rather than nothing: the call site's trailing semicolon must not
	// become an empty statement (warnings, and the same braceless-if hazard)
	#define RADIANT_ASSERT(...) ((void)0)
#endif
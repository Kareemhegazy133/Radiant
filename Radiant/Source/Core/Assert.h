#pragma once

#include "Core/Base.h"
#include "Core/Log.h"
#include <filesystem>

#ifdef RADIANT_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define RADIANT_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { RADIANT##type##ERROR(msg, __VA_ARGS__); RADIANT_DEBUGBREAK(); } }
	#define RADIANT_INTERNAL_ASSERT_WITH_MSG(type, check, ...) RADIANT_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define RADIANT_INTERNAL_ASSERT_NO_MSG(type, check) RADIANT_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", RADIANT_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define RADIANT_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define RADIANT_INTERNAL_ASSERT_GET_MACRO(...) RADIANT_EXPAND_MACRO( RADIANT_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, RADIANT_INTERNAL_ASSERT_WITH_MSG, RADIANT_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define RADIANT_ASSERT(...) RADIANT_EXPAND_MACRO( RADIANT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#else
	#define RADIANT_ASSERT(...)
#endif
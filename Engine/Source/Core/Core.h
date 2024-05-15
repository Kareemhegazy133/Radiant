#pragma once

#ifdef DEBUG
#define ENABLE_ASSERTS
#endif

#ifdef ENABLE_ASSERTS
#define GAME_ASSERT(x, ...) { if(!(x)) { GAME_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define ENGINE_ASSERT(x, ...) { if(!(x)) { ENGINE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#define GAME_ASSERT(x, ...)
#define ENGINE_ASSERT(x, ...)
#endif
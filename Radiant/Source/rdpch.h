#pragma once

#include "Core/PlatformDetection.h"

#ifdef RADIANT_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		// See github.com/skypjack/entt/wiki/Frequently-Asked-Questions#warning-c4003-the-min-the-max-and-the-macro
		#define NOMINMAX
	#endif
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>

// TODO:: Include it only where needed
#include <glm/glm.hpp>

#include "Core/Base.h"

#include "Core/Log.h"

#include "Debug/Instrumentor.h"

#ifdef RADIANT_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
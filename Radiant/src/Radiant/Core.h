#pragma once

#ifdef RD_PLATFORM_WINDOWS
	#ifdef RD_BUILD_DLL
		#define RADIANT_API __declspec(dllexport)
	#else
		#define RADIANT_API __declspec(dllimport)
	#endif
#else
	#error Radiant only supports the Windows platform!
#endif
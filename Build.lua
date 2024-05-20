workspace "TheReaper"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Game"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      defines { "_CRT_SECURE_NO_WARNINGS" }
	  disablewarnings { "4996" }
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["spdlog"] = "Engine/Vendor/spdlog/include"
IncludeDir["sfml"] = "Engine/Vendor/sfml/include"
IncludeDir["entt"] = "Engine/Vendor/entt/include"

project "Engine"
	location "Engine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	pchheader "%{prj.name}pch.h"
	pchsource "%{prj.name}/Source/%{prj.name}pch.cpp"

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/Source",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.sfml}",
		"%{IncludeDir.entt}"
	}

	libdirs
	{
		"%{prj.name}/Vendor/sfml/lib"
	}

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	filter "system:windows"
	systemversion "latest"

	filter "configurations:Debug"
		defines { "ENGINE_DEBUG" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "ENGINE_RELEASE" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines { "ENGINE_DIST" }
		runtime "Release"
		optimize "on"

project "Game"
	location "Game"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/Source",

		-- Include Engine
		"Engine/Source",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.sfml}",
		"%{IncludeDir.entt}"
	}

	libdirs
	{
		"Engine/Vendor/sfml/lib"
	}

	links
	{
		"Engine"
	}

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	filter "system:windows"
		systemversion "latest"
		defines { "WINDOWS" }

	filter "configurations:Debug"
		defines { "GAME_DEBUG" }
		runtime "Debug"
		symbols "on"
		
		links
		{
			"sfml-audio-d.lib",
			"sfml-graphics-d.lib",
			"sfml-network-d.lib",
			"sfml-system-d.lib",
			"sfml-window-d.lib"
		}
		
		postbuildcommands
		{
			"{COPY} %{prj.location}/../Engine/Vendor/sfml/dlls/Debug/*.dll %{cfg.targetdir}"
        }

	filter "configurations:Release"
		defines { "GAME_RELEASE" }
		runtime "Release"
		optimize "on"

		links
		{
			"sfml-audio.lib",
			"sfml-graphics.lib",
			"sfml-network.lib",
			"sfml-system.lib",
			"sfml-window.lib"
		}
		
		postbuildcommands
		{
			"{COPY} %{prj.location}/../Engine/Vendor/sfml/dlls/Release/*.dll %{cfg.targetdir}"
        }

	filter "configurations:Dist"
		defines { "GAME_DIST" }
		runtime "Release"
		optimize "on"

		links
		{
			"sfml-audio.lib",
			"sfml-graphics.lib",
			"sfml-network.lib",
			"sfml-system.lib",
			"sfml-window.lib"
		}
		
		postbuildcommands
		{
			"{COPY} %{prj.location}/../Engine/Vendor/sfml/dlls/Release/*.dll %{cfg.targetdir}"
        }

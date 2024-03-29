workspace "Radiant"
	architecture "x64"
	startproject "Sandbox"
	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Radiant/vendor/GLFW/include"
IncludeDir["Glad"] = "Radiant/vendor/Glad/include"
IncludeDir["ImGui"] = "Radiant/vendor/imgui"
IncludeDir["glm"] = "Radiant/vendor/glm"

include "Radiant/vendor/GLFW"
include "Radiant/vendor/Glad"
include "Radiant/vendor/imgui"

project "Radiant"
	location "Radiant"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	pchheader "rdpch.h"
	pchsource "Radiant/src/rdpch.cpp"
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
	}
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}
	
	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}"
	}
	
	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib",
		"dwmapi.lib"
	}
	
	filter "system:windows"
		systemversion "latest"
		
		defines
		{
			"RD_PLATFORM_WINDOWS",
			"RD_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}
		
	filter "configurations:Debug"
		defines "RD_DEBUG"
		runtime "Debug"
		symbols "on"
	
	filter "configurations:Release"
		defines "RD_RELEASE"
		runtime "Release"
		optimize "on"
		
	filter "configurations:Dist"
		defines "RD_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"Radiant/vendor/spdlog/include",
		"Radiant/src",
		"Radiant/vendor",
		"%{IncludeDir.glm}"
	}
	
	links
	{
		"Radiant"
	}
	
	filter "system:windows"
		systemversion "latest"
		
		defines
		{
			"RD_PLATFORM_WINDOWS"
		}
		
	filter "configurations:Debug"
		defines "RD_DEBUG"
		runtime "Debug"
		symbols "on"
	
	filter "configurations:Release"
		defines "RD_RELEASE"
		runtime "Release"
		optimize "on"
		
	filter "configurations:Dist"
		defines "RD_DIST"
		runtime "Release"
		optimize "on"
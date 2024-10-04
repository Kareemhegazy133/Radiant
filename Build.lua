workspace "Radiant"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Sandbox"
   
   flags
	{
		"MultiProcessorCompile"
	}

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      defines { "_CRT_SECURE_NO_WARNINGS" }
	  disablewarnings { "4996" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["spdlog"] = "Radiant/Vendor/spdlog/include"
IncludeDir["glm"] = "Radiant/Vendor/glm"
IncludeDir["glfw"] = "Radiant/Vendor/glfw/include"
IncludeDir["glad"] = "Radiant/Vendor/glad/include"
IncludeDir["stb_image"] = "Radiant/Vendor/stb_image"
IncludeDir["entt"] = "Radiant/Vendor/entt/include"
IncludeDir["box2d"] = "Radiant/Vendor/box2d/include"

group "Dependencies"
		include "Radiant/Vendor/glfw"
		include "Radiant/Vendor/glad"
		include "Radiant/Vendor/box2d"
group ""

project "Radiant"
	kind "StaticLib"
	location "Radiant"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	pchheader "rdpch.h"
	pchsource "%{prj.name}/Source/rdpch.cpp"

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp",
		"%{prj.name}/Vendor/glm/glm/**.hpp",
		"%{prj.name}/Vendor/glm/glm/**.inl",
		"%{prj.name}/Vendor/stb_image/**.h",
		"%{prj.name}/Vendor/stb_image/**.cpp"
	}
	
	defines
	{
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.name}/Source",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.glfw}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.box2d}"
	}
	
	links
	{
		"glfw",
		"glad",
		"box2d"
	}

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	filter "system:windows"
	systemversion "latest"

	filter "configurations:Debug"
		defines { "RD_DEBUG" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "RD_RELEASE" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines { "RD_DIST" }
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/Source",

		-- Include Radiant
		"Radiant/Source",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.box2d}"
	}
	
	links
	{
		"Radiant"
	}

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	filter "system:windows"
		systemversion "latest"
		defines { "WINDOWS" }

	filter "configurations:Debug"
		defines { "Sandbox_DEBUG" }
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "Sandbox_RELEASE" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines { "Sandbox_DIST" }
		runtime "Release"
		optimize "on"
workspace "Radiant"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "TheReaper"
   
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
IncludeDir["yaml_cpp"] = "Radiant/Vendor/yaml-cpp/include"
IncludeDir["msdf_atlas_gen"] = "Radiant/Vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["msdfgen"] = "Radiant/Vendor/msdf-atlas-gen/msdfgen"
IncludeDir["ImGui"] = "Radiant/Vendor/imgui"

group "Dependencies"
		include "Radiant/Vendor/glfw"
		include "Radiant/Vendor/glad"
		include "Radiant/Vendor/box2d"
		include "Radiant/Vendor/yaml-cpp"
		include "Radiant/Vendor/msdf-atlas-gen"
		include "Radiant/Vendor/imgui"
group ""

project "Radiant"
	kind "StaticLib"
	location "Radiant"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	pchheader "rdpch.h"
	pchsource "%{prj.name}/Source/rdpch.cpp"
	
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp",
		"%{prj.name}/Vendor/glm/glm/**.hpp",
		"%{prj.name}/Vendor/glm/glm/**.inl",
		"%{prj.name}/Vendor/stb_image/**.h",
		"%{prj.name}/Vendor/stb_image/**.cpp",
		
		"%{prj.name}/Vendor/imgui/misc/cpp/imgui_stdlib.h",
		"%{prj.name}/Vendor/imgui/misc/cpp/imgui_stdlib.cpp"
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
		"%{IncludeDir.box2d}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.ImGui}"
	}
	
	links
	{
		"glfw",
		"glad",
		"box2d",
		"yaml-cpp",
		"msdf-atlas-gen",
		"ImGui"
	}
	
	defines
	{
		"GLFW_INCLUDE_NONE",
		"YAML_CPP_STATIC_DEFINE"
	}
	
	filter "files:Radiant/Vendor/imgui/misc/cpp/imgui_stdlib.cpp"
	flags { "NoPCH" }

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
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

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
		"Radiant/Vendor",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.box2d}",
		"%{IncludeDir.yaml_cpp}"
	}
	
	links
	{
		"Radiant"
	}

	filter "system:windows"
		systemversion "latest"
		defines { "WINDOWS" }

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
		
project "TheReaper"
	location "TheReaper"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

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
		"Radiant/Vendor",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.box2d}",
		"%{IncludeDir.yaml_cpp}"
	}
	
	links
	{
		"Radiant"
	}

	filter "system:windows"
		systemversion "latest"
		defines { "WINDOWS" }

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
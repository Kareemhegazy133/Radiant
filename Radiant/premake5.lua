project "Radiant"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	pchheader "Radiant/rdpch.h"
	pchsource "Source/Radiant/rdpch.cpp"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Source/**.h",
		"Source/**.hpp",
		"Source/**.cpp",
		"Vendor/glm/glm/**.hpp",
		"Vendor/glm/glm/**.inl",
		"Vendor/stb_image/**.h",
		"Vendor/stb_image/**.cpp",

		"Vendor/imgui/misc/cpp/imgui_stdlib.h",
		"Vendor/imgui/misc/cpp/imgui_stdlib.cpp"
	}

	includedirs
	{
		"Source",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.glfw}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.box2d}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGui}"
	}

	links
	{
		"glfw",
		"glad",
		"box2d",
		"yaml-cpp",
		"ImGui"
	}

	defines
	{
		"GLFW_INCLUDE_NONE",
		"YAML_CPP_STATIC_DEFINE"
	}

	filter "files:**/imgui_stdlib.cpp"
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

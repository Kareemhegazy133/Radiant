project "Reaper"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	-- Assets load relative to the working directory (see Docs/Build-System.md)
	debugdir "%{prj.location}"

	files
	{
		"Source/**.h",
		"Source/**.hpp",
		"Source/**.cpp"
	}

	includedirs
	{
		"Source",

		-- Engine
		"%{wks.location}/Radiant/Source",
		"%{wks.location}/Radiant/Vendor",
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

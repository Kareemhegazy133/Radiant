workspace "Radiant"
	architecture "x64"
	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Radiant"
	location "Radiant"
	kind "SharedLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"%{prj.name}/vendor/spdlog/include"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		
		defines
		{
			"RD_PLATFORM_WINDOWS",
			"RD_BUILD_DLL"
		}
		
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}
		
	filter "configurations:Debug"
		defines "RD_DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "RD_RELEASE"
		optimize "On"
		
	filter "configurations:Dist"
		defines "RD_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}
	
	includedirs
	{
		"Radiant/vendor/spdlog/include",
		"Radiant/src"
	}
	
	links
	{
		"Radiant"
	}
	
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		
		defines
		{
			"RD_PLATFORM_WINDOWS"
		}
		
	filter "configurations:Debug"
		defines "RD_DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "RD_RELEASE"
		optimize "On"
		
	filter "configurations:Dist"
		defines "RD_DIST"
		optimize "On"
-- Workspace root: configuration and aggregation only.
-- Each project declares itself in its own premake5.lua (the premake analogue
-- of per-module build files); shared dependency paths live in Dependencies.lua.

include "Dependencies.lua"

workspace "Radiant"
	architecture "x64"
	configurations { "Debug", "Release", "Dist" }
	startproject "Reaper"

	flags
	{
		"MultiProcessorCompile"
	}

	-- Workspace-wide build options for MSVC
	filter "system:windows"
		defines { "_CRT_SECURE_NO_WARNINGS" }
		disablewarnings { "4996" }
	filter ""

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Radiant/Vendor/glfw"
	include "Radiant/Vendor/glad"
	include "Radiant/Vendor/box2d"
	include "Radiant/Vendor/yaml-cpp"
	include "Radiant/Vendor/imgui"
group ""

include "Radiant"
include "Reaper"

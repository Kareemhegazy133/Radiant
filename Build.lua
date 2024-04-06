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

group "Engine"
	include "Engine/Build-Engine.lua"
group ""

include "Game/Build-Game.lua"
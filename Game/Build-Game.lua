project "Game"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   staticruntime "on"

   files { "Source/**.h", "Source/**.hpp", "Source/**.cpp" }

   includedirs
   {
      "Source",

	  -- Include Engine
	  "../Engine/Source",
	  "../Engine/Vendor/spdlog/include"
   }

   links
   {
      "Engine"
   }
   
   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "on"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "on"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "on"
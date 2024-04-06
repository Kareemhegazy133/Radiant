project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++17"
   staticruntime "on"
   
   pchheader "%{prj.name}pch.h"
   pchsource "Source/%{prj.name}pch.cpp"

   files { "Source/**.h", "Source/**.hpp", "Source/**.cpp" }
 
   includedirs
   {
      "Source",
	  "Vendor/spdlog/include"
   }

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")
   
   filter "system:windows"
       systemversion "latest"
       defines { }

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

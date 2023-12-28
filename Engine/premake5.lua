project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   conformancemode (true)
   justmycode "Off"

   pchheader "EnginePCH.h"
   pchsource "Source/EnginePCH.cpp"

   
   targetdir ("Binaries/" .. OutputPath)
   objdir ("Intermediate/" .. OutputPath)



   files { "Source/**.h", "Source/**.cpp" }

   includedirs
   {
      "Source",
      "%{IncludeDirectories.glfw}",
      "%{IncludeDirectories.entt}",
      "%{IncludeDirectories.assimp}",
      "%{IncludeDirectories.stb_image}",
      "%{IncludeDirectories.spdlog}",
      "%{IncludeDirectories.DirectX}",
   }

   links
   {
      "%{Libraries.glfw}",
   }
   
   defines
   {
      "NOMINMAX",
      "SPDLOG_USE_STD_FORMAT",
      "SPDLOG_COMPILED_LIB",
   }

   filter "configurations:Debug"
      runtime "Debug"
      symbols "On"

   
   filter "configurations:Release"
      runtime "Release"
      optimize "On"
      symbols "Off"
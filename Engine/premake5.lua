project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "off"
   conformancemode (true)
   justmycode "Off"

   
   targetdir ("Binaries/" .. OutputPath)
   objdir ("Intermediate/" .. OutputPath)



   files { "Source/**.h", "Source/**.cpp" }

   includedirs
   {
      "%{IncludeDirectories.glfw}",
      "%{IncludeDirectories.entt}",
      "%{IncludeDirectories.assimp}",
      "%{IncludeDirectories.stb_image}",
      "%{IncludeDirectories.spdlog}",
   }

   links
   {
      "%{Libraries.glfw}",
   }
   
   defines
   {
      "NOMINMAX"
   }

   filter "configurations:Debug"
      runtime "Debug"
      symbols "On"

   
   filter "configurations:Release"
      runtime "Release"
      optimize "On"
      symbols "Off"
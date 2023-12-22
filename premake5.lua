
include "Defines.lua"

workspace "ToyProject"
	architecture "x86_64"
	startproject "Editor"
	configurations { "Debug", "Release" }
	flags { "MultiProcessorCompile" }


project "Editor"
   kind "WindowedApp"
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
      "Source",
      "Engine/Source",
      "%{IncludeDirectories.glfw}",
      "%{IncludeDirectories.entt}",
      "%{IncludeDirectories.assimp}",
      "%{IncludeDirectories.stb_image}",
      "%{IncludeDirectories.spdlog}",
      "%{IncludeDirectories.DirectX}",
      "%{IncludeDirectories.imgui}",
   }

   links
   {
      "Engine", 
      "%{Libraries.imgui}"
   }

   defines
   {
      "NOMINMAX"
   }

   filter "configurations:Debug"
      runtime "Debug"
      symbols "On"
      links
      {
         "%{Libraries.assimpd}",
         "%{Libraries.spdlogd}",
      }
      postbuildcommands
      {
         '{COPY} "%{Libraries.assimpd}.dll" "%{cfg.targetdir}"',
         '{COPY} "%{Libraries.spdlogd}.dll" "%{cfg.targetdir}"',
      }

   
   filter "configurations:Release"
      runtime "Release"
      optimize "On"
      symbols "Off"
      links
      {
         "%{Libraries.assimp}",
         "%{Libraries.spdlog}",
      }
      postbuildcommands
      {
         '{COPY} "%{Libraries.assimp}.dll" "%{cfg.targetdir}"',
         '{COPY} "%{Libraries.spdlog}.dll" "%{cfg.targetdir}"',
      }
   

include "Engine"
   


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
   staticruntime "Off"
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
      "%{IncludeDirectories.DirectXTK}",
      "%{IncludeDirectories.imgui}",
   }

   links
   {
      "Engine", 
      "imgui",
      "DirectXTK"
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
      links
      {
         "%{Libraries.assimpd}",
         "%{Libraries.spdlogd}",
      }
      postbuildcommands
      {
         '{COPY} "%{Libraries.assimpd}.dll" "%{cfg.targetdir}"',
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
      }
   

include "Engine"
group "ThirdParties"
   include "ThirdParties/imgui"
group ""
   

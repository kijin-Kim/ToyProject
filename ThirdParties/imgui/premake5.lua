project "imgui"
	kind "StaticLib"
	language "C++"
   cppdialect "C++17"
   staticruntime "Off"
   justmycode "Off"


   targetdir ("Binaries/" .. OutputPath)
   objdir ("Intermediate/" .. OutputPath)

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp",
      "misc/cpp/imgui_stdlib.h",
      "misc/cpp/imgui_stdlib.cpp",
      "misc/debuggers/imgui.natstepfilter",
      "misc/debuggers/imgui.natvis",
      "backends/imgui_impl_dx12.h",
      "backends/imgui_impl_dx12.cpp",
      "backends/imgui_impl_glfw.h",
      "backends/imgui_impl_glfw.cpp",  
	}

   includedirs
   {
      "%{prj.location}",
      "%{IncludeDirectories.glfw}",
   }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "Off"

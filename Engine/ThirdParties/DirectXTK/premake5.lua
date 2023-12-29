project "DirectXTK"
	kind "StaticLib"
	language "C++"
   	cppdialect "C++20"
   	staticruntime "Off"
	justmycode "Off"

	targetdir ("Binaries/" .. OutputPath)
	objdir ("Intermediate/" .. OutputPath)

	files { "**.h", "**.cpp", "**.inl" }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "Off"

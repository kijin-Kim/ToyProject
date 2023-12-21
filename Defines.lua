IncludeDirectories = {}
IncludeDirectories["glfw"] = "%{wks.location}/Engine/ThirdParties/glfw/include"
IncludeDirectories["entt"] = "%{wks.location}/Engine/ThirdParties/entt-3.12.2/single_include"
IncludeDirectories["assimp"] = "%{wks.location}/Engine/ThirdParties/assimp/include"
IncludeDirectories["stb_image"] = "%{wks.location}/Engine/ThirdParties/stb_image"
IncludeDirectories["spdlog"] = "%{wks.location}/Engine/ThirdParties/spdlog/include"


OutputPath = "%{cfg.buildcfg}/%{cfg.system}/%{cfg.architecture}"

LibraryDirectories = {}
LibraryDirectories["glfw"] = "%{wks.location}/Engine/ThirdParties/glfw/Binaries/%{cfg.buildcfg}/"
LibraryDirectories["spdlog"] = "%{wks.location}/Engine/ThirdParties/spdlog/Binaries/%{cfg.buildcfg}/"
LibraryDirectories["assimp"] = "%{wks.location}/Engine/ThirdParties/assimp/Binaries/%{cfg.buildcfg}/"

Libraries = {}
Libraries["glfw"] = "%{LibraryDirectories.glfw}/glfw3"
Libraries["spdlogd"] = "%{LibraryDirectories.spdlog}/spdlogd"
Libraries["spdlog"] = "%{LibraryDirectories.spdlog}/spdlog"
Libraries["assimpd"] = "%{LibraryDirectories.assimp}/assimp-vc143-mtd"
Libraries["assimp"] = "%{LibraryDirectories.assimp}/assimp-vc143-mt"

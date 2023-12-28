#pragma once

#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXMath.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <entt/entt.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <wrl/client.h>
#include "d3dx12.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
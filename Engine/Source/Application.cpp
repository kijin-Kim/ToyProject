#include "Application.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

Application::Application(ApplicationSpec applicationSpec)
	: m_ApplicationSpec(std::move(applicationSpec))
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_Window = glfwCreateWindow(m_ApplicationSpec.Width, m_ApplicationSpec.Height, m_ApplicationSpec.WindowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(m_Window);

}

Application::~Application()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();

}

void Application::Run()
{
	Initialize();
	const uint64_t tickPerSec = glfwGetTimerFrequency();
	float lastTime = glfwGetTimerValue() / static_cast<float>(tickPerSec);

	while (!glfwWindowShouldClose(m_Window))
	{
		const float currentTime = glfwGetTimerValue() / static_cast<float>(tickPerSec);
		m_DeltaTime = currentTime - lastTime;
		lastTime = currentTime;
		glfwPollEvents();
		Update();
	}
}

void Application::Update()
{
}

HWND Application::GetWindowHandle() const
{
	return glfwGetWin32Window(m_Window);
}

float Application::GetDeltaTime() const
{
	return m_DeltaTime;
}

#include "Window.h"

#include <Render/Swapchain.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace anv {

	Window::Window(WindowCreateInfo _info)
	{
		if (!glfwInit())
		{
			throw std::runtime_error("Failed to init GLFW!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // create a window with no context
		m_WinPtr = glfwCreateWindow(_info.width, _info.height, _info.name.c_str(), NULL, NULL);

		if (!m_WinPtr)
			throw std::runtime_error("Window Creation Failed!");

		m_Context = Context::Create(this); // Create and initialize a rendering context
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_WinPtr);
		glfwTerminate();
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();

	}

	bool Window::ShouldClose()
	{
		return glfwWindowShouldClose(m_WinPtr);
	}

	Context& Window::GetContext()
	{
		return *m_Context;
	}
	Swapchain& Window::GetSwapChain()
	{
		return *m_Swapchain;
	}

}
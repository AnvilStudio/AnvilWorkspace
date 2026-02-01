#include "Window.h"

#include "../Render/Swapchain.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace anv {
	_shared<Window> Window::Create(WindowCreateInfo _info)
	{
		return std::make_shared<Window>(_info);
	}

	Window::Window(WindowCreateInfo _info)
	{
		if (!glfwInit())
		{
			ANV_LOG_FATAL("Failed to initialize glfw!")
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // create a window with no context
		m_WinPtr = glfwCreateWindow(_info.width, _info.height, _info.name.c_str(), NULL, NULL);

		if (!m_WinPtr)
			ANV_LOG_FATAL("Failed to Create window: %s!", _info.name.c_str())

		m_Context = Context::Create(this); // Create and initialize a rendering context
		m_Context->CreateSwapchain();   
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

	_shared<Context> Window::GetContext()
	{
		return m_Context;
	}

	Extent Window::GetExtent()
	{
		Extent e;
		glfwGetWindowSize(m_WinPtr, &e.width, &e.height);
		return e;
	}

	bool Window::GetKeyState(int _code)
	{
		auto state = glfwGetKey(m_WinPtr, _code);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Window::GetMouseButtonState(int _button)
	{
		auto state = glfwGetMouseButton(m_WinPtr, _button);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Window::GetMousePos()
	{
		double x, y;
		glfwGetCursorPos(m_WinPtr, &x, &y);
		return { x, y };
	}
}

#include "GLFWInputSystem.h"
#include <GLFW/glfw3.h>

anv::GLFWInputSystem::GLFWInputSystem(_shared<Window> _win)
	: InputSystem(_win)
{
	glfwSetScrollCallback(_win->GetNativeWindow(),
		[](GLFWwindow* window, double xOffset, double yOffset)
		{
			s_MouseScrollX += static_cast<float>(xOffset);
			s_MouseScrollY += static_cast<float>(yOffset);
		}
	);
}

float anv::GLFWInputSystem::getMouseScrollX()
{
	return s_MouseScrollX;
}

float anv::GLFWInputSystem::getMouseScrollY()
{
	return s_MouseScrollY;
}

void anv::GLFWInputSystem::resetScroll()
{
	s_MouseScrollX = 0.0f;
	s_MouseScrollY = 0.0f;
}

bool anv::GLFWInputSystem::is_key_pressed(int _code)
{
	return m_Window->GetKeyState(_code);
}

bool anv::GLFWInputSystem::is_mouse_button_pressed(int _button)
{
	return m_Window->GetMouseButtonState(_button);
}

std::pair<float, float> anv::GLFWInputSystem::get_mouse_pos()
{
	return m_Window->GetMousePos();
}

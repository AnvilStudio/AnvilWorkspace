#include "GLFWInputSystem.h"

anv::GLFWInputSystem::GLFWInputSystem(_shared<Window> _win)
	: InputSystem(_win)
{

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

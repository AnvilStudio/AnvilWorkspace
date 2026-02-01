#include "Input.h"
#include "Platform/GLFWInputSystem.h"

namespace anv {
	_shared<InputSystem> InputSystem::Create(_shared<Window> _win)
	{
		s_InputSystem = std::make_shared<GLFWInputSystem>(_win);
		return s_InputSystem;
	}

	InputSystem::InputSystem(_shared<Window> _win)
		: m_Window(_win)
	{
		ANV_LOG_INFO("Initializing input system")
	}
}
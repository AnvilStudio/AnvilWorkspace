#include "Input.h"
#include "Platform/GLFWInputSystem.h"

namespace anv
{
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

	void InputSystem::Update()
	{
		m_PreviousKeys = m_CurrentKeys;

		for (int key = 0; key <= ANV_KEY_LAST; ++key)
		{
			if (!IsValidKeyCode(key))
				continue;

			m_CurrentKeys[key] = is_key_pressed(key);
		}
	}

	KeyState InputSystem::GetKeyState(int _code) const
	{
		if (!IsValidKeyCode(_code))
			return KeyState::Up;

		const bool current = m_CurrentKeys[_code];
		const bool previous = m_PreviousKeys[_code];

		if (current && !previous)
			return KeyState::Pressed;
		if (current && previous)
			return KeyState::Held;
		if (!current && previous)
			return KeyState::Released;

		return KeyState::Up;
	}

	bool InputSystem::IsKeyPressed(int _code) const
	{
		const KeyState state = GetKeyState(_code);
		return state == KeyState::Pressed || state == KeyState::Held;
	}

	bool InputSystem::IsKeyJustPressed(int _code) const
	{
		return GetKeyState(_code) == KeyState::Pressed;
	}

	bool InputSystem::IsKeyHeld(int _code) const
	{
		return GetKeyState(_code) == KeyState::Held;
	}

	bool InputSystem::IsKeyReleased(int _code) const
	{
		return GetKeyState(_code) == KeyState::Released;
	}

	bool InputSystem::IsValidKeyCode(int _code)
	{
		return _code == ANV_KEY_SPACE ||
			_code == ANV_KEY_APOSTROPHE ||
			(_code >= ANV_KEY_COMMA && _code <= ANV_KEY_9) ||
			_code == ANV_KEY_SEMICOLON ||
			_code == ANV_KEY_EQUAL ||
			(_code >= ANV_KEY_A && _code <= ANV_KEY_RIGHT_BRACKET) ||
			_code == ANV_KEY_GRAVE_ACCENT ||
			(_code >= ANV_KEY_WORLD_1 && _code <= ANV_KEY_WORLD_2) ||
			(_code >= ANV_KEY_ESCAPE && _code <= ANV_KEY_END) ||
			(_code >= ANV_KEY_CAPS_LOCK && _code <= ANV_KEY_PAUSE) ||
			(_code >= ANV_KEY_F1 && _code <= ANV_KEY_F25) ||
			(_code >= ANV_KEY_KP_0 && _code <= ANV_KEY_KP_EQUAL) ||
			(_code >= ANV_KEY_LEFT_SHIFT && _code <= ANV_KEY_MENU);
	}
}

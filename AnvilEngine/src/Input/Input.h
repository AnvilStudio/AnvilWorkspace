#pragma once
#include "Keys.h"
#include "../Util/UMacros.h"
#include "../Core/Window.h"

#include <array>

namespace anv
{
	enum class KeyState
	{
		Up = 0,
		Pressed,
		Held,
		Released
	};

	class InputSystem
	{
	public:
		static _shared<InputSystem> Create(_shared<Window> _win);
		InputSystem(_shared<Window> _win);

		void Update();

		KeyState GetKeyState(int _code) const;
		bool IsKeyPressed(int _code) const;
		bool IsKeyJustPressed(int _code) const;
		bool IsKeyHeld(int _code) const;
		bool IsKeyReleased(int _code) const;

		bool IsMouseButtonPressed(int _button) { return s_InputSystem->is_mouse_button_pressed(_button); }
		std::pair<float, float> GetMousePos() { return s_InputSystem->get_mouse_pos(); }
		float GetMouseScrollX() { return s_InputSystem->getMouseScrollX(); }
		float GetMouseScrollY() { return s_InputSystem->getMouseScrollY(); }
		void ResetScroll() { s_InputSystem->resetScroll(); }

	protected:
		virtual bool is_key_pressed(int _code) = 0;
		virtual bool is_mouse_button_pressed(int _button) = 0;
		virtual std::pair<float, float> get_mouse_pos() = 0;
		virtual float getMouseScrollX() = 0;
		virtual float getMouseScrollY() = 0;
		virtual void resetScroll() = 0;

		_shared<Window> m_Window;

	private:
		static bool IsValidKeyCode(int _code);

		inline static _shared<InputSystem> s_InputSystem = nullptr;
		std::array<bool, ANV_KEY_LAST + 1> m_CurrentKeys{};
		std::array<bool, ANV_KEY_LAST + 1> m_PreviousKeys{};
	};
}

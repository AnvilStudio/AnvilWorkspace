#pragma once
#include "Keys.h"
#include "../Util/UMacros.h"
#include "../Core/Window.h"

namespace anv
{
	class InputSystem
	{
	public:
		static _shared<InputSystem> Create(_shared<Window> _win);
		InputSystem(_shared<Window> _win);

		bool IsKeyPressed(int _code) { return s_InputSystem->is_key_pressed(_code); }
		bool IsMouseButtonPressed(int _button) { return s_InputSystem->is_mouse_button_pressed(_button); }
		std::pair<float, float> GetMousePos() { return s_InputSystem->get_mouse_pos(); }

	protected:
		virtual bool is_key_pressed(int _code) = 0;
		virtual bool is_mouse_button_pressed(int _button) = 0;
		virtual std::pair<float, float> get_mouse_pos() = 0;


		_shared<Window> m_Window;

	private:
		inline static _shared<InputSystem> s_InputSystem = nullptr;
	};
}
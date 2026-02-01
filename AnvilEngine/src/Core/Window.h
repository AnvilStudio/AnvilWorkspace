#pragma once

/**
* Note:
* OS window abstraction will happen within glfw.
* idc about OS window abstraction. i care about
* Graphics API abstraction and modularity.
**/

#include "../Render/Context.h"
#include <string>

struct GLFWwindow;

namespace anv {

	struct WindowCreateInfo
	{
		int width, height;
		std::string name = "Anvil App Window";
	};

	struct Extent
	{
		int width, height;
	};

	class Window
	{
	public:
		static _shared<Window> Create(WindowCreateInfo _info);

		Window(WindowCreateInfo _info);
		~Window();

		void OnUpdate();
		Extent GetExtent();
		bool ShouldClose();

		GLFWwindow* GetNativeWindow() { return m_WinPtr; }
		_shared<Context> GetContext();

		bool GetKeyState(int _code);
		bool GetMouseButtonState(int _button);
		std::pair<float, float> GetMousePos();

	private:
		GLFWwindow*        m_WinPtr    = nullptr;
		_shared<Context>   m_Context   = nullptr;
	};

}
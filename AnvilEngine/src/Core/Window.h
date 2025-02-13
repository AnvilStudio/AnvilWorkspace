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

	class Window
	{
	public:
		static _shared<Window> Create(WindowCreateInfo _info);

		Window(WindowCreateInfo _info);
		~Window();

		void OnUpdate();
		bool ShouldClose();

		GLFWwindow* GetNativeWindow() { return m_WinPtr; }
		_shared<Context> GetContext();

	private:
		GLFWwindow*        m_WinPtr    = nullptr;
		_shared<Context>   m_Context   = nullptr;
	};

}
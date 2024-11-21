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

	class Swapchain;

	struct WindowCreateInfo
	{
		int width, height;
		std::string name = "Anvil App Window";
	};

	class Window
	{
	public:
		Window(WindowCreateInfo _info);
		~Window();

		void OnUpdate();
		bool ShouldClose();

		GLFWwindow* GetNativeWindow() { return m_WinPtr; }
		Context& GetContext();

	private:
		GLFWwindow*        m_WinPtr  = nullptr;
		_shared<Context>   m_Context = nullptr;	
	};

}
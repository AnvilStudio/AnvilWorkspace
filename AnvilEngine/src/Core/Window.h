#pragma once

/**
* Note:
* OS window abstraction will happen within glfw.
* idc about OS window abstraction. i care about
* Graphics API abstraction and modularity.
**/

#include "../Render/Context/Context.h"
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

		Context& GetContext();
		Swapchain& GetSwapChain();

	private:
		GLFWwindow* m_WinPtr   = nullptr;
		
		_unique(Context)   m_Context = nullptr;
		_unique(Swapchain) m_Swapchain = nullptr;
		
		friend class VulkanContext;
	};

}
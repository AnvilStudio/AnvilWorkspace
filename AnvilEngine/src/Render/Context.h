#pragma once
#include "../Util/UMacros.h"
#include "Swapchain.h"
#include "RenderAPI.h"

struct GLFWwindow;

namespace anv {

	class Window;

	
	/// <summary>
	/// initializes the Graphics API,
	/// Creates the swapchain
	/// </summary>
	class Context
	{

	public:
		static _shared<Context> Create(Window* _win);
		virtual ~Context() = default;

		_shared<RenderAPI> InitAPI(RenderAPICreateInfo _info);

		virtual void CreateSwapchain() = 0;

		inline Ref<Swapchain> GetSwapchain()
		{
			return m_Swapchain;
		}

		inline _shared<RenderAPI> GetAPI()
		{
			// Dynamic cast because API class is pure virtual
			return m_API;
		}

		template<typename T>
		inline T* GetAs()
		{
			return static_cast<T*>(this);
		}

		Context(Window* _win);
	
	protected:
		GLFWwindow*        m_WinHandle;
		Ref<Swapchain>     m_Swapchain;
		_shared<RenderAPI> m_API;
	};
}

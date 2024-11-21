#pragma once
#include "../Util/UMacros.h"
#include "Swapchain.h"
#include "RenderAPI.h"

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
		
		Context();

		_shared<RenderAPI> InitAPI(RenderAPICreateInfo _info);

		template<typename T>
		_shared<T> GetNativeAPIAs();

		template<typename T>
		T* GetNativeContextAs();

	private:
		_shared<RenderAPI> m_API;
	};

	template<typename T>
	inline T* Context::GetNativeContextAs()
	{
		return static_cast<T*>(this);
	}

	template<typename T>
	inline _shared<T> Context::GetNativeAPIAs()
	{
		// Dynamic cast because API class is pure virtual
		return dynamic_cast<T*>(m_API);
	}
}

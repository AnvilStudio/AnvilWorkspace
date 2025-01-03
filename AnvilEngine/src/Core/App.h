#pragma once
#include "../Render/Renderer.h"
#include "Window.h"
#include <string>

namespace anv
{	
	struct AppCreateInfo
	{
		std::string name = "Anvil App";
		std::string version = "";
		std::string description = "";

		WindowCreateInfo WindowCreateInfo;
	};

	class App
	{
	public:
		App();
		App(AppCreateInfo _info);
		~App();

		void Run();

		static App* GetInstance();
		_shared<Window> GetMainWindow();

	public:
		virtual void OnSetup()   {};
		virtual void OnUpdate()  {};
		virtual void OnDestroy() {};

	private:
		inline static App* m_This  = nullptr;
		_shared<Window> m_AppWin   = nullptr;
	};

}
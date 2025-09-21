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
		App(AppCreateInfo _info, int arg_c = 0, char* arg_v[] = nullptr);
		~App();

		void Run();

		static App* GetInstance();
		_shared<Window> GetMainWindow();

	public:
		virtual void OnSetup()   {};
		virtual void OnUpdate()  {};
		virtual void OnDestroy() {};

	private:
		void NavigateToProjectDir(std::string path);

	private:
		std::string m_ProjPath;
		inline static App* m_This  = nullptr;
		_shared<Window> m_AppWin   = nullptr;
	};

}
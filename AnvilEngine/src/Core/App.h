#pragma once
#include "../Render/Renderer.h"
#include "../Scene/Scene.h"
#include "../Scene/Manager.h"
#include "Window.h"
#include <string>

namespace anv
{	

	struct AppSettings
	{
		std::string projectName;
		std::string projectPath;
		std::string projectDir;
		std::string version = "";
		std::string description = "";
		std::string startScene = "";
		WindowCreateInfo WindowCreateInfo;
	};

	class App
	{
	public:
		App();
		App(int arg_c = 0, char* arg_v[] = nullptr);
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
		void PopulateSettings(const std::string& prjPath);

	private:
		AppSettings   m_Settings;
		inline static App* m_This  = nullptr;
		SceneManager    m_ScnMngr;
		_shared<Window> m_AppWin   = nullptr;
	};

}
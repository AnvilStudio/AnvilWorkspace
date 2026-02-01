#pragma once
#include "../Render/Renderer.h"
#include "../Asset/AssetManager.h"
#include "../Scene/Scene.h"
#include "../Scene/Manager.h"
#include "../Input/Input.h"
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

		std::string engineRes = "";
		std::string assetDir = "";
		std::string cacheDir = "";

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
		FileSystem& GetFS();
		_shared<Window> GetMainWindow();

	public:
		virtual void OnSetup()   {};
		virtual void OnUpdate()  {};
		virtual void OnDestroy() {};

	private:
		void SaveStates();
		void InitializeFileSys();
		void PopulateSettings(const std::string prjPath);

	private:
		_shared<InputSystem> m_InputSystem;
		std::unique_ptr<FileSystem> m_FileSystem;
		AppSettings        m_Settings;
		SceneManager       m_ScnMngr;
		inline static App* m_This  = nullptr;
		_shared<Window>    m_AppWin   = nullptr;
		_shared<AssetManager>    m_AssetManager = nullptr;
	};

}
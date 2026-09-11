#pragma once
#include "../Render/Renderer.h"
#include "../Asset/AssetManager.h"
#include "../Layer/LayerStack.h"
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

		std::string assetDir     = "Assets/";
		std::string engineRes  = "Assets/com.anvstu.engine/";
		std::string cacheDir    = "Assets/com.anvstu.engine/Cache";
		std::string assetMeta  =  "Assets/com.anvstu.engine/AssetMeta";
		std::string settings     = "Assets/com.anvstu.engine/Settings";

		WindowCreateInfo WindowCreateInfo;
	};

	struct AppStats
	{
		float frameTime = 0.f;
		FPSCounter fps;
		
	};

	struct EngineInfo
	{
		std::string version = ANV_ENGINE_VERSION;
		std::string graphicsAPI = ANV_API_VER;
	};

	class App
	{
	public:
		App();
		App(int arg_c = 0, char* arg_v[] = nullptr);
		virtual ~App() noexcept;

		void Run();

		static App*             GetInstance();
		FileSystem&            GetFS();
		_shared<Window> GetMainWindow();
		_shared<AssetManager> GetAssetManager();
		_shared<SceneManager> GetSceneManager();
		_shared<InputSystem>   GetInputSystem();
		AppStats& GetStats() { return m_Stats; }
		EngineInfo& GetEngineInfo() {return m_EngineInfo;}

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopOverlay(Layer* overlay);

		void Close();

	public:
		virtual void OnSetup()     = 0;
		virtual void OnUpdate()   = 0;
		virtual void OnDestroy()  = 0;

	protected:
		void SaveStates();
		void InitializeFileSys();
		void PopulateSettings(const std::string prjPath);

	protected:
		inline static App*   s_This  = nullptr;

		AppSettings           m_Settings;
		EngineInfo            m_EngineInfo;
		LayerStack             m_LayerStack;
		_unique<FileSystem>        m_FileSystem    = nullptr;
		_shared<SceneManager>  m_ScnMngr = nullptr;
		_shared<InputSystem>     m_InputSystem = nullptr;
		_shared<Window>            m_AppWin        = nullptr;
		_shared<AssetManager>   m_AssetManager = nullptr;
		
		AppStats m_Stats;
		bool m_CloseEvent = false;
	};

}
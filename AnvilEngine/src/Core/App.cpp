#include "App.h"
#include <filesystem>
#include <Util/Serialize/Serializer.h>
#include <Util/Time/Time.h>
#include "AppLayers/SceneLayer.h"
// tmp
#include "../Asset/AssetTypes/Texture.h"

namespace anv
{

	App::App()
	{
		if (s_This == nullptr)
			s_This = this;

		// init logging
		{
			anv_log::LogCreateInfo info{
				.logFilePath = "logs.alog",
				.timeFormat = "%I:%M:%S",
				.consoleOutput = true,
				.fileOutput = true,
				.abortOnError = false};

			anv_log::AnvLog::Init(info);
		}

		ANV_PROFILE_SCOPE()

		WindowCreateInfo i{
			.width = 800,
			.height = 500,
			.name = "My Window"};

		m_AppWin = Window::Create(i);
	}

	App::App(int arg_c, char *arg_v[])
	{
		ANV_PROFILE_SCOPE()
		if (s_This == nullptr)
			s_This = this;

		// Navigate to project directory
		if ((arg_c - 1) <= 0)
		{
			ANV_LOG_INFO("== Usage ==");
			ANV_LOG_INFO("--projectPath (-prj) <path>\t...\tWhere your .anv project is");
			std::exit(EXIT_SUCCESS);
		}

		for (int i = 0; i < arg_c; i++)
		{
			if ((strcmp(arg_v[i], "--projectPath") == 0 ||
				 strcmp(arg_v[i], "-prj") == 0) &&
				i + 1 < arg_c)
			{
				m_Settings.projectPath = arg_v[i + 1];
				break;
			}
		}

		anv_log::LogCreateInfo info{
			.logFilePath = m_Settings.projectPath,
			.logFileName = ".alog",
			.timeFormat = "%I:%M:%S",
			.consoleOutput = true,
			.fileOutput = true,
			.abortOnError = false};

		anv_log::AnvLog::Init(info);

		PopulateSettings(m_Settings.projectPath);
		InitializeFileSys();

		// 1. Window
		m_AppWin = Window::Create(m_Settings.WindowCreateInfo);

		// 2. Input and time
		m_InputSystem = InputSystem::Create(m_AppWin);
		Time::Init();

		// 3. Renderer selects Metal
		Render2DCreateInfo renderInfo{};
		renderInfo.pTarget = m_AppWin;
		Renderer2D::Init(renderInfo);

		// 4. Restore assets, including Metal textures
		m_AssetManager = std::make_shared<AssetManager>();

		// 5. Restore scene and its texture UUID references
		m_ScnMngr = std::make_shared<SceneManager>();
		m_ScnMngr->Register(m_Settings.startScene);

		// 6. Editor/game layers
		PushLayer(new SceneLayer(m_ScnMngr));
	}

	App::~App() noexcept
	{
		SaveStates();

		for (Layer *layer : m_LayerStack)
		{
			layer->OnDetach();
		}

		// Everything should be deleted before the app itself gets deleted
		Renderer2D::Shutdown();
		m_ScnMngr->Shutdown();
	}

	void App::Run()
	{
		s_This->OnSetup();

		while (!m_AppWin->ShouldClose() && m_CloseEvent == false)
		{
			TIME_SCOPE(m_Stats.frameTime)

			Time::Update();
			m_Stats.fps.Update(Time::DeltaTime());

			m_InputSystem->ResetScroll();

			// Poll input/events
			m_AppWin->OnUpdate();
			m_InputSystem->Update();

			// Update engine/game layers
			for (Layer *layer : m_LayerStack)
				layer->OnUpdate(Time::DeltaTime());

			Renderer2D::BeginScene();
			s_This->OnUpdate();

			// Render layers
			for (Layer *layer : m_LayerStack)
				layer->OnRender();

			// Render UI
			for (Layer *layer : m_LayerStack)
				layer->OnImGuiRender();

			Renderer2D::EndScene();
			Renderer2D::DrawFrame();
		}
		s_This->OnDestroy();
	}

	App *App::GetInstance()
	{
		return s_This;
	}

	_shared<Window> App::GetMainWindow()
	{
		return m_AppWin;
	}

	_shared<AssetManager> App::GetAssetManager()
	{
		return m_AssetManager;
	}

	_shared<SceneManager> App::GetSceneManager()
	{
		return m_ScnMngr;
	}

	_shared<InputSystem> App::GetInputSystem()
	{
		return m_InputSystem;
	}

	void App::PushLayer(Layer *layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void App::PopLayer(Layer *layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnDetach();
	}
	void App::PushOverlay(Layer *overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void App::PopOverlay(Layer *overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnDetach();
	}

	void App::Close()
	{
		m_CloseEvent = true;
	}

	// Serialize all settings
	void App::SaveStates()
	{
		Serializer ser(s_This->m_Settings.projectPath,
					   Serializer::Mode::SER_MODE_TOML,
					   Serializer::Direction::Write);

		ser.Object("Settings", [&]
				   {
				ser.Field("ProjName", m_Settings.projectName);
				ser.Field("ProjDir", m_Settings.projectDir);
				ser.Field("Version", m_Settings.version);
				ser.Field("Description", m_Settings.description);

				ser.Object("StartScene", [&]
					{
						auto active = m_ScnMngr->GetActive();

						if (!active)
						{
							ANV_LOG_ERROR("'SceneManager->GetActive() returned nullptr!'")
						}
						else {
							auto name = active->GetName();
							auto UUID = active->GetUUID();
							auto pth = active->GetPath();
							ser.Field("Name", name);
							ser.Field("UUID", UUID.uuid);
							ser.Field("Path", pth);
						}
					});

				ser.Object("WindowInfo", [&] {

					int w = m_AppWin->GetExtent().width;
					int h = m_AppWin->GetExtent().height;
					ser.Field("Width", w);
					ser.Field("Height", h);
					});

				// --- directories ---
				ser.ObjectIf("Directories", [&] {

					std::string assets = m_FileSystem->GetKeyVal("Assets").string();
					std::string res = m_FileSystem->GetKeyVal("Res").string();
					std::string cache = m_FileSystem->GetKeyVal("Cache").string();
					std::string ameta = m_FileSystem->GetKeyVal("AssetMeta").string();                                  
					std::string settings = m_FileSystem->GetKeyVal("Settings").string();

					ser.FieldOr<std::string>("Assets",  assets, "Assets/");
					ser.FieldOr<std::string>("EngineRes", res, "Assets/com.anvstu.engine/");
					ser.FieldOr<std::string>("Cache", cache, "Assets/com.anvstu.engine/Cache/");
					ser.FieldOr<std::string>("AssetMeta", ameta, "Assets/com.anvstu.engine/AssetMeta");
					ser.FieldOr<std::string>("Settings", settings, "Assets/com.anvstu.engine/Settings");

				}); });

		ser.Close();
	}

	void App::PopulateSettings(const std::string prjPath)
	{
		try
		{
			Serializer ser(prjPath, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Read);

			ser.ObjectStrict("Settings", [&]
							 {
					// --- Basic metadata ---
					ser.FieldOr<std::string>("ProjName", m_Settings.projectName, "");
					ser.FieldOr<std::string>("ProjDir", m_Settings.projectDir, "");
					ser.FieldOr<std::string>("Version", m_Settings.version, "");
					ser.FieldOr<std::string>("Description", m_Settings.description, "");

					// --- Window ---
					ser.ObjectIf("WindowInfo", [&]
						{
							ser.FieldOr("Width", m_Settings.WindowCreateInfo.width, 1280);
							ser.FieldOr("Height", m_Settings.WindowCreateInfo.height, 720);
						});

					// --- directories ---
					ser.ObjectIf("Directories", [&] {
						ser.FieldOr<std::string>("Assets",       m_Settings.assetDir, "Assets/");
						ser.FieldOr<std::string>("EngineRes",  m_Settings.engineRes, "Assets/com.anvstu.engine/");
						ser.FieldOr<std::string>("Cache",        m_Settings.cacheDir, "Assets/com.anvstu.engine/Cache/");
						ser.FieldOr<std::string>("AssetMeta", m_Settings.assetMeta, "Assets/com.anvstu.engine/AssetMeta");
						ser.FieldOr<std::string>("Settings",     m_Settings.settings, "Assets/com.anvstu.engine/Settings");

						});

					// --- Start scene ---
					ser.ObjectIf("StartScene", [&]
						{
							std::string path;

							ser.FieldOr<std::string>("Path", path, "");
							m_Settings.startScene = path;
						}); });

			ser.Close();

			m_Settings.projectPath = prjPath;

			std::string win_name = m_Settings.projectName;
			win_name.append(" | ");
			win_name.append(ANV_ENGINE_VERSION);
			win_name.append(" | Anvil Engine | ");
			win_name.append(ANV_API_VER);
			m_Settings.WindowCreateInfo.name = win_name;
		}
		catch (const std::exception &e)
		{
			ANV_LOG_FATAL("Failed to load project settings: %s", e.what());
		}
	}

	FileSystem &App::GetFS()
	{
		ANV_ASSERT(m_FileSystem, "Cannot retrieve file system when app is not initialized!");
		return *m_FileSystem;
	}

	void App::InitializeFileSys()
	{
		m_FileSystem = std::make_unique<FileSystem>(m_Settings.projectDir);

		// Get Asset dir
		std::filesystem::path assetPath(m_Settings.projectDir);
		
		m_FileSystem->MountKey("Assets", (assetPath / "Assets").c_str());
		m_FileSystem->MountKey("Res", "@Assets/com.anvstu.engine");
		m_FileSystem->MountKey("Cache", "@Res/Cache");
		m_FileSystem->MountKey("ShaderCache", "@Cache/ShaderCache");
		m_FileSystem->MountKey("ShaderLib", "@Res/ShaderLib");
		m_FileSystem->MountKey("AssetMeta", "@Res/AssetMeta");
		m_FileSystem->MountKey("Setting", "@Res/Settings");
	}
}
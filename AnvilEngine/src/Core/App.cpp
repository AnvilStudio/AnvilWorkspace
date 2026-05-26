#include "App.h"	
#include <filesystem>
#include <Util/Serialize/Serializer.h>
#include <Util/Time/Time.h>

//tmp
#include "../Asset/AssetTypes/Texture.h" 

namespace anv {

	App::App()
	{
		if (s_This == nullptr)
			s_This = this;

		// init logging
		{
			anv_log::LogCreateInfo info
			{
				.logFilePath = "logs.alog",
				.timeFormat = "%I:%M:%S",
				.consoleOutput = true,
				.fileOutput = true,
				.abortOnError = false
			};

			anv_log::AnvLog::Init(info);
		}

		ANV_PROFILE_SCOPE()


		WindowCreateInfo i{
		.width = 800,
		.height = 500,
		.name = "My Window"
		};

		m_AppWin = Window::Create(i);

	}

	App::App(int arg_c, char* arg_v[])
	{
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
			if ((strcmp(arg_v[i], "--projectPath") || strcmp(arg_v[i], "-prj")) && i + 1 < arg_c)
			{
				i += 2;
				m_Settings.projectPath = arg_v[i];
				break;
			}
			else {
				ANV_LOG_FATAL("Please specify a project path")
			}
		}

		ANV_PROFILE_SCOPE()

		anv_log::LogCreateInfo info
		{
			.logFilePath = m_Settings.projectPath,
			.logFileName = ".alog",
			.timeFormat = "%I:%M:%S",
			.consoleOutput = true,
			.fileOutput = true,
			.abortOnError = false
		};

		anv_log::AnvLog::Init(info);


		PopulateSettings(m_Settings.projectPath);

		InitializeFileSys();

		m_ScnMngr = std::make_unique<SceneManager>();
		m_ScnMngr->Register(m_Settings.startScene);

		// window needs to be created before assets.
		m_AppWin = Window::Create(m_Settings.WindowCreateInfo);

		m_AssetManager = std::make_shared<AssetManager>();
		m_InputSystem = InputSystem::Create(m_AppWin);

		// Test
		//auto t_path = m_FileSystem->GetKeyVal("Assets") / "TestText.png";
		//Ref<Texture> text = m_AssetManager->Create<Texture>(t_path);

		Time::Init();

		// FIX: prototyping...
		Render2DCreateInfo r_info{};
		r_info.pTarget = m_AppWin;

		Renderer2D::Init(r_info);
	}

	App::~App()
	{
		ANV_PROFILE_SCOPE()
		SaveStates();

		// Everything should be deleted before the app itself gets deleted
		Renderer2D::Shutdown();
		m_ScnMngr->Shutdown();
	}

	void App::Run()
	{
		s_This->OnSetup();
		auto mainCamera = m_ScnMngr->GetActive()->GetMainCamera();

		Renderer2D::SetCamera(mainCamera);

		int dur = 0;

		while (!m_AppWin->ShouldClose())
		{
			dur++;
			Time::Update();
			m_FPS.Update(Time::DeltaTime());
			if (dur == 2000)
			{
				ANV_LOG_DEBUG("FPS: %i", m_FPS.GetFPS());
				dur = 0;
			}

			// Polls input
			m_AppWin->OnUpdate();

			// OnUpdate should hapen after input polling
			s_This->OnUpdate();

			m_ScnMngr->GetActive()->OnUpdate(Time::DeltaTime());

			Renderer2D::BeginScene();

			Renderer2D::DrawQuad(
				{ 0.0f, -1.0f },     // top
				{ 1.f, 1.f },
				{ 1, 0, 0, 1 }
			);

			Renderer2D::DrawQuad(
				{ -1.0f, 1.0f },   // bottom left
				{ 1.f, 1.f },
				{ 0, 1, 0, 1 }
			);

			Renderer2D::DrawQuad(
				{ 1.0f, 1.0f },    // bottom right
				{ 1.f, 1.f },
				{ 0, 0, 1, 1 }
			);

			Renderer2D::EndScene();
			Renderer2D::DrawFrame();
		}
		ANV_LOG_INFO("App Closing...")
		s_This->OnDestroy();
	}

	App* App::GetInstance()
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

	_shared<InputSystem> App::GetInputSystem()
	{
		return m_InputSystem;
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
				});
			});

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
						});
				});

			ser.Close();

			m_Settings.projectPath = prjPath;

			std::string win_name = m_Settings.projectName;
			win_name.append(" | ver ");
			win_name.append(m_Settings.version);
			win_name.append(" | Anvil Engine");
			m_Settings.WindowCreateInfo.name = win_name;
		}
		catch (const std::exception& e)
		{
			ANV_LOG_FATAL("Failed to load project settings: %s", e.what());
		}
	}

	FileSystem& App::GetFS() 
	{ 
		ANV_ASSERT(m_FileSystem, "Cannot retrieve file system when app is not initialized!"); 
		return *m_FileSystem; 
	}


	void App::InitializeFileSys()
	{
		m_FileSystem = std::make_unique<FileSystem>(m_Settings.projectDir);
		m_FileSystem->MountKey("Assets", m_Settings.assetDir);
		m_FileSystem->MountKey("Res", "@Assets/com.anvstu.engine");
		m_FileSystem->MountKey("Cache", "@Res/Cache");
		m_FileSystem->MountKey("ShaderCache", "@Cache/ShaderCache");
		m_FileSystem->MountKey("ShaderLib", "@Res/ShaderLib");
		m_FileSystem->MountKey("AssetMeta", "@Res/AssetMeta");
		m_FileSystem->MountKey("Setting", "@Res/Settings");
	}
}
#include "App.h"	
#include <filesystem>
#include <Util/Serialize/Serializer.h>

namespace anv {

	App::App()
	{
		if (m_This == nullptr)
			m_This = this;

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

		// Setup should happen after App setup
		OnSetup();
	}

	App::App(int arg_c, char* arg_v[])
	{
		if (m_This == nullptr)
			m_This = this;

		// Navigate to project directory
		if ((arg_c - 1) <= 0)
		{
			ANV_LOG_INFO("== Usage ==");
			ANV_LOG_INFO("--projectPath (-prj) <path>\t...\tWhere your .anv project is");
			std::exit(EXIT_SUCCESS);
		}

		ANV_LOG_DEBUG("Arg Count: %i", arg_c);

		for (int i = 0; i < arg_c; i++)
		{
			ANV_LOG_DEBUG("%i> %s", i, arg_v[i]);
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

		m_AppWin = Window::Create(m_Settings.WindowCreateInfo);

		// TODO: prototyping...
		Render2DCreateInfo r_info{};
		r_info.pTarget = m_AppWin;

		Renderer2D::Init(r_info);

		// Jump to client side setup
		OnSetup();
	}

	App::~App()
	{
		ANV_PROFILE_SCOPE()
		SaveStates();
		OnDestroy();

		// Everything should be deleted before the app itself gets deleted
		Renderer2D::Shutdown();
	}

	void App::Run()
	{
		while (!m_AppWin->ShouldClose())
		{
			// Polls input
			m_AppWin->OnUpdate();

			// OnUpdate should hapen after input polling
			OnUpdate();
		}
	}

	App* App::GetInstance()
	{
		return m_This;
	}

	_shared<Window> App::GetMainWindow()
	{
		return m_AppWin;
	}

	// Serialize all settings
	void App::SaveStates()
	{
		Serializer ser(m_Settings.projectPath, 
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
						auto name = m_ScnMngr.GetActive()->GetName();
						auto UUID = m_ScnMngr.GetActive()->GetUUID();
						auto pth = m_ScnMngr.GetActive()->GetPath();
						ser.Field("Name", name);
						ser.Field("UUID", UUID.uuid);
						ser.Field("Path", pth);
					});

				ser.Object("WindowInfo", [&] {

					int w = m_AppWin->GetExtent().width;
					int h = m_AppWin->GetExtent().height;
					ser.Field("Width", w);
					ser.Field("Height", h);
					});

				// --- directories ---
				ser.ObjectIf("Directories", [&] {
					ser.Field("EngineRes", m_Settings.assetDir);
					ser.Field("Assets", m_Settings.assetDir);
					ser.Field("Cache", m_Settings.cacheDir);
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
						ser.FieldOr<std::string>("EngineRes", m_Settings.assetDir, "Assets/com.anvstu.engine/");
						ser.FieldOr<std::string>("Assets", m_Settings.assetDir, "Assets/");
						ser.FieldOr<std::string>("Cache", m_Settings.cacheDir, "Assets/com.anvstu.engine/Cache/");
						});

					// --- Start scene ---
					ser.ObjectIf("StartScene", [&]
						{
							std::string path;

							ser.FieldOr<std::string>("Path", path, "");
							m_Settings.startScene = path;

							if (!path.empty())
							{
								m_ScnMngr.Register(path);
							}
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
		m_FileSystem->CreateKeyDir("Assets", m_Settings.assetDir);
		m_FileSystem->CreateKeyDir("AnvRes", m_Settings.engineRes);
		m_FileSystem->CreateKeyDir("AnvCache", m_Settings.cacheDir);
	}
}
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
		if (arg_c <= 0)
		{
			ANV_LOG_INFO("== Usage ==");
			ANV_LOG_INFO("--projectPath (-prj) <path>\t...\tWhere your .anv project is");
			std::exit(EXIT_SUCCESS);
		}

		for (int i = 0; i < arg_c; i++)
		{
			std::string arg = std::string(arg_v[i]);
			if ((arg == "--projectPath" || arg == "-prj") && i + 1 < arg_c)
			{
				m_Settings.projectPath = arg_v[i + 1];
				{
					anv_log::LogCreateInfo info
					{
						.logFilePath = m_Settings.projectPath,
						.logFileName = "forgelog.alog",
						.timeFormat = "%I:%M:%S",
						.consoleOutput = true,
						.fileOutput = true,
						.abortOnError = false
					};

					anv_log::AnvLog::Init(info);
				}

				PopulateSettings(m_Settings.projectPath);
				NavigateToProjectDir(m_Settings.projectDir);
				break;
			}
		}

		ANV_PROFILE_SCOPE()

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

		Serializer ser(m_Settings.projectPath, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Write);
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

			});
		ser.Close();
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

	void App::NavigateToProjectDir(std::string path)
	{
		if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
		{
			std::filesystem::current_path(path);
			ANV_LOG_INFO("Changed working directory to: " + path);
		}
		else
		{
			ANV_LOG_FATAL("Project path does not exist or is not a directory: " + path);
		}
	}

	void App::PopulateSettings(const std::string& prjPath)
	{
		using anv::Serializer;

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

					// --- Window (optional) ---
					ser.ObjectIf("WindowInfo", [&]
						{
							ser.FieldOr("Width", m_Settings.WindowCreateInfo.width, 1280);
							ser.FieldOr("Height", m_Settings.WindowCreateInfo.height, 720);
						});

					// --- Start scene (optional) ---
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
}
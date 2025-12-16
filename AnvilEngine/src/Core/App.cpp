#include "App.h"	
#include <filesystem>
#include <toml++/toml.hpp>

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
		toml::table tbl;

		try
		{
			tbl = toml::parse_file(prjPath); // IMPORTANT
		}
		catch (const toml::parse_error& e)
		{
			ANV_LOG_FATAL("Failed to parse project file: %s", prjPath.c_str());
			return;
		}

		const auto settings = tbl["Settings"].as_table();
		if (!settings)
		{
			ANV_LOG_FATAL("Missing [Settings] table");
			return;
		}

		// --- Basic metadata ---
		m_Settings.projectName =
			(*settings)["ProjName"].value_or("");

		m_Settings.projectDir =
			(*settings)["ProjDir"].value_or("");

		m_Settings.version =
			(*settings)["Version"].value_or("");

		m_Settings.description =
			(*settings)["Description"].value_or("");

		m_Settings.projectPath = prjPath;

		// --- Window ---
		if (const auto win = (*settings)["WindowInfo"].as_table())
		{
			m_Settings.WindowCreateInfo.width =
				(*win)["Width"].value_or(1280);

			m_Settings.WindowCreateInfo.height =
				(*win)["Height"].value_or(720);

			std::string win_name = m_Settings.projectName;
			win_name.append(" | ver ");
			win_name.append(m_Settings.version);
			win_name.append(" | Anvil Engine");
			m_Settings.WindowCreateInfo.name = win_name;
		}

		// --- Start scene ---
		if (const auto scene = (*settings)["StartScene"].as_table())
		{
			std::string sceneName =
				(*scene)["Name"].value_or("");

			std::string uuid =
				(*scene)["UUID"].value_or("");

			if (!sceneName.empty())
			{
				// however you create/load scenes
				//m_Settings.startScene = Scene::Create(sceneName, uuid);
			}
		}
	}
}
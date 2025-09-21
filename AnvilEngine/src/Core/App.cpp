#include "App.h"	
#include <filesystem>

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

	App::App(AppCreateInfo _info, int arg_c, char* arg_v[])
	{
		if (m_This == nullptr)
			m_This = this;

		// Navigate to project directory
		for (int i = 0; i < arg_c; i++)
		{
			std::string arg = std::string(arg_v[i]);
			if (arg == "-projectPath" && i + 1 < arg_c)
			{
				m_ProjPath = arg_v[i + 1];
				NavigateToProjectDir(m_ProjPath);
				break;
			}
		}

		// init logging
		{
			anv_log::LogCreateInfo info
			{
				.logFilePath = "logs.alog",
				.timeFormat = "%I:%M:%S",
				.logFilePath = m_ProjPath,
				.consoleOutput = true,
				.fileOutput = true,
				.abortOnError = false
			};

			anv_log::AnvLog::Init(info);
		}

		ANV_PROFILE_SCOPE()

		m_AppWin = Window::Create(_info.WindowCreateInfo);

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
}
#include "App.h"
#include <Util/CrashHandler/CrashHandler.h>	

namespace anv {

	App::App()
	{
		if (m_This == nullptr)
			m_This = this;

		// init logging
		{
			anv_log::LogCreateInfo info
			{
				.logFilePath = "AnvLogs.log",
				.timeFormat = "%I:%M:%S",
				.consoleOutput = true,
				.fileOutput = true
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
		ANV_LOG_INFO("==========================================\n============= SETUP COMPLETE =============\n==========================================")
	}

	App::App(AppCreateInfo _info)
	{
		if (m_This == nullptr)
			m_This = this;

		// init logging
		{
			anv_log::LogCreateInfo info
			{
				.logFilePath = "AnvLogs.log",
				.timeFormat = "%I:%M:%S",
				.consoleOutput = true,
				.fileOutput = true
			};

			anv_log::AnvLog::Init(info);
		}

		ANV_PROFILE_SCOPE()

		m_AppWin = Window::Create(_info.WindowCreateInfo);

		Render2DCreateInfo r_info{};
		r_info.pTarget = m_AppWin;

		m_Renderer = new Renderer2D(r_info);
		OnSetup();
	}

	App::~App()
	{
		OnDestroy();

		// Everything should be deleted before the app itself gets deletes
	}

	void App::Run()
	{
		while (!m_AppWin->ShouldClose())
		{
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
}
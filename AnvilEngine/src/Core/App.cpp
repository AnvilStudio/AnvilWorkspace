#include "App.h"

namespace anv {



	App::App()
	{
		if (m_This == nullptr)
			m_This = this;

		WindowCreateInfo i{
		.width = 800,
		.height = 500,
		.name = "My Window"
		};

		m_AppWin = new Window(i);

		// Setup should happen after App setup
		OnSetup();
	}

	App::App(AppCreateInfo _info)
	{
		if (m_This == nullptr)
			m_This = this;

		m_AppWin = new Window(_info.WindowCreateInfo);

		OnSetup();
	}

	App::~App()
	{
		OnDestroy();

		// Everything should be deleted before the app itself gets deletes
		delete m_AppWin;
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

	Window* App::GetMainWindow()
	{
		return m_AppWin;
	}
}
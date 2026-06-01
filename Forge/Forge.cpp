#include "Anvil.h"
#include "Entry.h"
#include "Editor/EditorLayer.h"

class Forge : public anv::App
{
public:

	Forge(int arg_c, char* arg_v[])
		: App(arg_c, arg_v)
	{
	}

	inline void OnSetup()   override 
	{
		ANV_LOG_INFO("hello from forge!");
		App::PushOverlay(new EditorLayer());
	}

	inline void OnUpdate()  override 
	{
	}

	inline void OnDestroy() override
	{
	}

	friend class EditorLayer;
};

anv::App* CreateApp(int arg_c, char* arg_v[])
{
	return new Forge(arg_c, arg_v);
}



#include "Anvil.h"
#include "Entry.h"

class Forge : public anv::App
{
public:
	Forge(int arg_c, char* arg_v[])
		: App(arg_c, arg_v)
	{
		// Initialize Forge specific components here if needed
	}

	void OnSetup()   override 
	{
		ANV_LOG_INFO("hello from forge!");

	};

	void OnUpdate()  override {
	};

	void OnDestroy() override {};

};

anv::App* CreateApp(int arg_c, char* arg_v[])
{
	return new Forge(arg_c, arg_v);
}



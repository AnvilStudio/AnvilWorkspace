#include "Anvil.h"
#include "Entry.h"

class Forge : public anv::App
{
public:
	Forge(anv::AppCreateInfo _info, int arg_c, char* arg_v[])
		: App(_info, arg_c, arg_v)
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
	anv::AppCreateInfo i
	{
		.name = "ForgeEditor",
		.version = "dev 1.0.0",
		.description = "Level editor for anvil",
	};

	i.WindowCreateInfo.name = "Level Editor";
	i.WindowCreateInfo.width = 750;
	i.WindowCreateInfo.height = 500;

	return new Forge(i, arg_c, arg_v);
}



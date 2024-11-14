#include "Anvil.h"
#include "Entry.h"

class Forge : public anv::App
{
public:
	Forge(anv::AppCreateInfo _info);

	void OnSetup()   override {};
	void OnUpdate()  override {};
	void OnDestroy() override {};

};

Forge::Forge(anv::AppCreateInfo _info)
	: App(_info)
{

}

anv::App* CreateApp()
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

	return new Forge(i);
}



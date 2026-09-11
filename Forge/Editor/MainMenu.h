#pragma once
#include <Anvil.h>
#include "Windows/ProjectSettings.h"
#include "Windows/About.h"

using namespace anv;
class MainMenu
{
public:
    void Draw();

private:
    ProjectSettings m_PrjSettingsWindow;
    About m_AboutWindow;
    
};

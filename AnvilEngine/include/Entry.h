#pragma once
#include <iostream>
#include "../src/Core/Macros.h"
#include "Anvil.h"   

extern anv::App* CreateApp();

#ifdef PLATFORM_WIN64
#ifdef DEBUG

int main(int arg_c, char* arg_v[])
{
    anv::App* app = CreateApp();

    app->Run();

    delete app;
}

#endif

#ifdef RELEASE
int WinMain()
{
    anv::App* app = CreateApp();
    try {
        app->Run();
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << '\n';
    }

    delete app;
}
#endif // RELEASE
#else // Platform ?
int main(int arg_c, char* arg_v[])
{
    anv::App* app = CreateApp();

    app->Run();

    delete app;
}
#endif
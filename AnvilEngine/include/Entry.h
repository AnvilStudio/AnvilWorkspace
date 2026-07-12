#pragma once
#include <iostream>
#include "../src/Core/Macros.h"
#include "Anvil.h"   

extern anv::App* CreateApp(int arg_c = 0, char* arg_v[] = nullptr);

#ifdef PLATFORM_WIN64
    #ifdef DEBUG
        int main(int arg_c, char* arg_v[])
        {
            ANV_LOG_DEBUG("Arg Count: %i", arg_c);

            for (int i = 0; i < arg_c; i++)
            {
                ANV_LOG_DEBUG("%i> %s", i, arg_v[i]);
            }
            
            anv::App* app = CreateApp(arg_c, arg_v);

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
#endif

#ifdef PLATFORM_APPLE
int main(int arg_c, char* arg_v[])
{
    std::fprintf(stderr, "MAIN: creating app\n");
    std::fflush(stderr);

    std::unique_ptr<anv::App> app(CreateApp(arg_c, arg_v));

    if (!app)
    {
        std::fprintf(stderr, "MAIN: CreateApp returned nullptr\n");
        return 1;
    }

    std::fprintf(stderr, "MAIN: entering Run\n");
    std::fflush(stderr);

    app->Run();

    std::fprintf(stderr, "MAIN: Run returned\n");
    std::fflush(stderr);

    app.reset();

    std::fprintf(stderr, "MAIN: app destroyed\n");
    std::fflush(stderr);

    return 0;
}
#endif
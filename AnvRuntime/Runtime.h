#include <Anvil>
#include <Entry.h>

class AnvRuntime : public anv::App
{
    public:
        AnvRuntime(int arg_c, char* arg_v[])
            : App(arg_c, arg_v)
        {

        }

        inline void OnSetup() override
        {
            const std::WorkingDir = 
                m_Settings.prjectDir.empty() ?
                std::filesystem::path(m_Settings.projectPath).parent_path()
                : std::filesystem::path(m_Settings.projectDir);

            anv::PythonScriptEngine::Initialize(WorkingDir);
        }

        inline void OnDestroy() override
        {
            anv::PythonScriptEngine::Shutdown();
        }
};

anv::App* CreateApp(int arg_c, char* arg_v[])
{
    return new AnvRuntime(arg_c, arg_v);
};
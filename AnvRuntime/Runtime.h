
#include <Anvil.h>
#include <Entry.h>

#include <filesystem>

class AnvRuntime : public anv::App
{
public:
    AnvRuntime(int argc, char *argv[])
        : anv::App(argc, argv)
    {
    }

    void OnSetup() override
    {
        const std::filesystem::path workingDir =
            m_Settings.projectDir.empty()
                ? std::filesystem::path(m_Settings.projectPath).parent_path()
                : std::filesystem::path(m_Settings.projectDir);

        anv::PythonScriptEngine::Initialize(workingDir);
    }

    void OnUpdate() override
    {
    }

    void OnDestroy() override
    {
        anv::PythonScriptEngine::Shutdown();
    }

    void Play();
    void discard_snapshot();

private:
    std::filesystem::path m_OriginalScenePath;
    std::filesystem::path m_ResolvedScenePath;
    std::filesystem::path m_SnapshotPath;
};

anv::App *CreateApp(int argc, char *argv[])
{
    return new AnvRuntime(argc, argv);
}


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

        if (!anv::PythonScriptEngine::Initialize(workingDir))
            ANV_LOG_ERROR("Runtime Python initialization failed for '%s'.",
                          workingDir.string().c_str());

        auto sceneManager = GetSceneManager();
        auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
        if (!scene)
        {
            ANV_LOG_ERROR("Runtime has no startup scene.");
            Close();
            return;
        }

        scene->SetScriptExecutionEnabled(true);
        scene->StartPhysics();
        ANV_LOG_INFO("Runtime started scene '%s'.", scene->GetName().c_str());
    }

    void OnUpdate() override
    {
    }

    void OnDestroy() override
    {
        auto sceneManager = GetSceneManager();
        auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
        if (scene)
        {
            scene->SetScriptExecutionEnabled(false);
            scene->StopPhysics();
        }
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

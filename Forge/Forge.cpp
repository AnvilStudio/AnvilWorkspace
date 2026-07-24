#include "Anvil.h"
#include "Entry.h"
#include "Editor/EditorLayer.h"
#include "Editor/InspectorExtensionLayer.h"

#include <filesystem>

class Forge : public anv::App
{
public:
    Forge(int arg_c, char* arg_v[])
        : App(arg_c, arg_v)
    {
    }

    inline void OnSetup() override
    {
        ANV_LOG_INFO("hello from forge!");

        const std::filesystem::path projectDirectory =
            m_Settings.projectDir.empty()
                ? std::filesystem::path(m_Settings.projectPath).parent_path()
                : std::filesystem::path(m_Settings.projectDir);

        anv::PythonScriptEngine::Initialize(projectDirectory);

        auto* editorLayer = new EditorLayer();
        App::PushOverlay(editorLayer);

        // Registered after EditorLayer so the Files panel creates its current
        // drag/drop payload before the Inspector accepts it.
        App::PushOverlay(new InspectorExtensionLayer(editorLayer));
    }

    inline void OnUpdate() override
    {
        // Entity scripts are updated by Scene::OnUpdate so each scene controls
        // the lifecycle and timing of its own script instances.
    }

    inline void OnDestroy() override
    {
        anv::PythonScriptEngine::Shutdown();
    }

    friend class EditorLayer;
};

anv::App* CreateApp(int arg_c, char* arg_v[])
{
    return new Forge(arg_c, arg_v);
}

#pragma once
#include <Anvil.h>
#include "Windows/CodeEditor.h"
#include "Windows/FileBrowser.h"
#include "Windows/Viewport.h"
#include "Windows/AssetRegistry.h"
#include "Windows/ConsolePanel.h"
#include "Windows/GameViewport.h"

#include <algorithm>
#include <cctype>

struct Windows
{
    bool showAssetRegistry = true;
    bool showStats = true;
    bool showCodeEditor = true;
    bool showConsole = true;
};

class EditorLayer : public anv::Layer
{
public:
    enum class SceneState
    {
        Edit,
        Play,
        Pause
    };

    EditorLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnImGuiRender() override;

    entt::entity GetSelectedEntity() const { return m_SelectedEntity; }
    static SceneState GetSceneState() { return m_SceneState; }
    static EditorLayer* GetInstance() {return m_This;}
    void ClearSelection() { m_SelectedEntity = entt::null; }

private:
    void begin_dock_space();
    void draw_menu_bar();
    void draw_scene_hierarchy();
    void draw_inspector();
    void draw_add_component_menu(
        anv::Ref<anv::Scene> scene,
        entt::entity entity);
    void draw_stats();

    entt::entity m_SelectedEntity = entt::null;
    entt::entity m_EntityToDelete = entt::null;

    static SceneState m_SceneState;

    Viewport m_Viewport;
    CodeEditorPanel m_DevNotes;
    FileBrowser m_FileBrowser;
    AssetRegistryPanel m_AssetRegistryPanel;
    GameViewport m_GameViewport;
    anv::ConsolePanel m_Console;

    static EditorLayer* m_This;

    Windows m_Windows{false};
};

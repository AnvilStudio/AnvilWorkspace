#pragma once
#include <Anvil.h>
#include "Windows/CodeEditor.h"
#include "Windows/FileBrowser.h"
#include "Windows/Viewport.h"
#include "Windows/AssetRegistry.h"
#include "Windows/ConsolePanel.h"

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
    SceneState GetSceneState() const { return m_SceneState; }
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

    SceneState m_SceneState = SceneState::Edit;

    Viewport m_Viewport;
    CodeEditorPanel m_CodeEditor;
    FileBrowser m_FileBrowser;
    AssetRegistryPanel m_AssetRegistryPanel;
    anv::ConsolePanel m_Console;

    Windows m_Windows{false};
};

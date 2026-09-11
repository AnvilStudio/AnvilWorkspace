#pragma once
#include <Anvil.h>

#include "Windows/AssetRegistry.h"
#include "Windows/CodeEditor.h"
#include "Windows/ConsolePanel.h"
#include "Windows/FileBrowser.h"
#include "Windows/GameViewport.h"
#include "Windows/InspectorLayer.h"
#include "Windows/SceneHierarchy.h"
#include "Windows/Viewport.h"
#include "MainMenu.h"

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
    void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }
    void ClearSelection() { m_SelectedEntity = entt::null; }

    static SceneState GetSceneState() { return m_SceneState; }
    void SetSceneState(SceneState state) { m_SceneState = state; }
    static EditorLayer *GetInstance() { return m_This; }

    Windows& GetWindowSettings() {return m_Windows;}

private:
    void begin_dock_space();
    void draw_menu_bar();
    void draw_stats();

private:
    entt::entity m_SelectedEntity = entt::null;

    static SceneState m_SceneState;
    static EditorLayer *m_This;

    MainMenu m_MainMenu;
    Viewport m_Viewport;
    CodeEditorPanel m_DevNotes;
    FileBrowser m_FileBrowser;
    AssetRegistryPanel m_AssetRegistryPanel;
    GameViewport m_GameViewport;
    SceneHierarchy m_SceneHierarchy;
    InspectorLayer m_InspectorLayer;
    anv::ConsolePanel m_Console;

    Windows m_Windows{};
};

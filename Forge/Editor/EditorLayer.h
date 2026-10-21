#pragma once
#include <Anvil.h>
#include <algorithm>
#include <vector>

#include "Windows/AssetRegistry.h"
#include "Windows/CodeEditor.h"
#include "Windows/ConsolePanel.h"
#include "Windows/FileBrowser.h"
#include "Windows/GameViewport.h"
#include "Windows/InspectorLayer.h"
#include "Windows/SceneHierarchy.h"
#include "Windows/SpritesheetEditor.h"
#include "Windows/Viewport.h"
#include "MainMenu.h"

struct Windows
{
    bool showViewport = true;
    bool showGameViewport = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showFiles = true;
    bool showAssetRegistry = false;
    bool showStats = false;
    bool showCodeEditor = false;
    bool showConsole = true;
    bool showSpritesheetEditor = false;

    void ShowAll()
    {
        showViewport = showGameViewport = showHierarchy = showInspector =
            showFiles = showAssetRegistry = showStats = showCodeEditor = showConsole = showSpritesheetEditor = true;
    }

    void RestoreDefaults() { *this = Windows{}; }
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
    const std::vector<entt::entity>& GetSelectedEntities() const { return m_SelectedEntities; }
    bool IsEntitySelected(entt::entity entity) const
    {
        return std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end();
    }
    void SetSelectedEntity(entt::entity entity)
    {
        m_SelectedEntity = entity;
        m_SelectedEntities.clear();
        if (entity != entt::null) m_SelectedEntities.push_back(entity);
    }
    void ToggleSelectedEntity(entt::entity entity)
    {
        auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
        if (it == m_SelectedEntities.end())
        {
            m_SelectedEntities.push_back(entity);
            m_SelectedEntity = entity;
        }
        else
        {
            m_SelectedEntities.erase(it);
            m_SelectedEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
        }
    }
    void SetSelectedEntities(std::vector<entt::entity> entities)
    {
        m_SelectedEntities = std::move(entities);
        m_SelectedEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
    }
    void ClearSelection() { m_SelectedEntities.clear(); m_SelectedEntity = entt::null; }
    void RemoveFromSelection(entt::entity entity)
    {
        auto it = std::remove(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
        m_SelectedEntities.erase(it, m_SelectedEntities.end());
        m_SelectedEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
    }

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
    std::vector<entt::entity> m_SelectedEntities;

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
    SpritesheetEditor m_SpritesheetEditor;
    anv::ConsolePanel m_Console;

    Windows m_Windows{};
    std::string m_ImGuiIniPath;
};

#include "EditorLayer.h"

using namespace anv;

EditorLayer::SceneState EditorLayer::m_SceneState = SceneState::Edit;
EditorLayer* EditorLayer::m_This = nullptr;

EditorLayer::EditorLayer()
    : anv::Layer("Editor Layer"),
      m_DevNotes(anv::App::GetInstance()->GetFS().GetKeyVal("Assets") / "Notes.toml"),
      m_FileBrowser(anv::App::GetInstance()->GetFS().GetKeyVal("Assets")),
      m_SceneHierarchy(anv::App::GetInstance()->GetSceneManager()->GetActive())
{
    m_This = this;
}

void EditorLayer::OnAttach()
{
    ImGuiIO& io = ImGui::GetIO();

    auto& fs = App::GetInstance()->GetFS();
    auto path = fs.GetKeyVal("Assets") / "Fonts/JetBrainsMono-Bold.ttf";

    io.Fonts->AddFontFromFileTTF(path.string().c_str(), 18.0f);
    io.ConfigDpiScaleFonts = true;

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 3.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.TabRounding = 2.0f;
    style.GrabRounding = 2.0f;

    style.WindowPadding = ImVec2(2, 2);
    style.FramePadding = ImVec2(8, 4);
    style.CellPadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 13.0f;
    style.GrabMinSize = 10.0f;

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.145f, 0.145f, 0.149f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.12f, 0.12f, 0.12f, 0.8f);
    colors[ImGuiCol_Text] = ImVec4(0.831f, 0.831f, 0.831f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.208f, 0.478f, 0.741f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_Tab] = ImVec4(0.145f, 0.145f, 0.149f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.208f, 0.478f, 0.741f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.110f, 0.110f, 0.110f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.145f, 0.145f, 0.149f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.149f, 0.310f, 0.471f, 1.0f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.290f, 0.565f, 0.886f, 0.7f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.208f, 0.478f, 0.741f, 1.0f);

    anv_log::AnvLog::SetCallback(
        [this](const anv_log::LogRecord& record)
        {
            m_Console.AddLogRecord(record);
        });

    ANV_LOG_INFO("Forge console connected to AnvLog");
}

void EditorLayer::OnUpdate(float dt)
{
    (void)dt;
}

void EditorLayer::OnImGuiRender()
{
    begin_dock_space();
    draw_menu_bar();

    auto sceneManager = App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;

    m_SceneHierarchy.Draw();
    m_FileBrowser.Draw();
    m_InspectorLayer.Draw(scene, m_SelectedEntity);
    draw_stats();
    m_DevNotes.OnImGuiRender(&m_Windows.showCodeEditor);
    m_AssetRegistryPanel.Draw(&m_Windows.showAssetRegistry);
    m_Console.Draw(&m_Windows.showConsole);
    m_GameViewport.Draw();
    m_Viewport.Draw();
}

void EditorLayer::OnDetach()
{
    anv_log::AnvLog::ClearCallback();
}

void EditorLayer::draw_stats()
{
    if (!m_Windows.showStats)
        return;

    ImGui::Begin("Stats");
    auto& stats = App::GetInstance()->GetStats();
    ImGui::Text("FPS: %u", stats.fps.GetFPS());
    ImGui::Text("Frame Time: %.3f ms", stats.frameTime);
    ImGui::End();
}

void EditorLayer::begin_dock_space()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Dockspace", nullptr, flags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceID = ImGui::GetID("AnvilDockspace");
    ImGui::DockSpace(
        dockspaceID,
        ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void EditorLayer::draw_menu_bar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            auto sceneManager = App::GetInstance()->GetSceneManager();
            auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
            if (scene)
                scene->Save();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Reload Scene"))
            App::GetInstance()->GetSceneManager()->ReloadActive();

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
            App::GetInstance()->Close();

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Asset Registry", nullptr, &m_Windows.showAssetRegistry);
        ImGui::MenuItem("Dev Notes", nullptr, &m_Windows.showCodeEditor);
        ImGui::MenuItem("Stats", nullptr, &m_Windows.showStats);
        ImGui::MenuItem("Console", nullptr, &m_Windows.showConsole);
        ImGui::EndMenu();
    }

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 80.0f) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.55f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.35f, 0.18f, 1.0f));

    if (m_SceneState == SceneState::Edit)
    {
        if (ImGui::Button("Play"))
            m_SceneState = SceneState::Play;
    }
    else
    {
        if (ImGui::Button("Stop"))
            m_SceneState = SceneState::Edit;
    }

    ImGui::PopStyleColor(3);
    ImGui::EndMainMenuBar();
}

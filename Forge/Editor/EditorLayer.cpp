#include "EditorLayer.h"
#include "MainMenu.h"
#include <algorithm>
#include <cstdio>

using namespace anv;

EditorLayer::SceneState EditorLayer::m_SceneState = SceneState::Edit;
EditorLayer *EditorLayer::m_This = nullptr;

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
    ImGuiIO &io = ImGui::GetIO();

    auto &fs = App::GetInstance()->GetFS();
    io.IniFilename = (fs.GetKeyVal("Settings") / "EditorConfig" / "EditorLayout.ini").string().c_str();
    auto path = fs.GetKeyVal("Assets") / "Fonts" / "JetBrainsMono-Bold.ttf";

    io.Fonts->AddFontFromFileTTF(path.string().c_str(), 18.0f);
    io.ConfigDpiScaleFonts = true;

    ImGuiStyle &style = ImGui::GetStyle();

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

    ImVec4 *colors = style.Colors;

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
        [this](const anv_log::LogRecord &record)
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
    m_MainMenu.Draw();

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

    // Save ImGui Layout
    // auto& fs = App::GetInstance()->GetFS();
    // auto IniPath = fs.GetKeyVal("Settings") / "EditorConfig" / "EditorLayout.ini";
    // ImGui::SaveIniSettingsToDisk(IniPath.string().c_str());
}

void EditorLayer::draw_stats()
{
    if (!m_Windows.showStats)
        return;

    if (ImGui::Begin("Stats", &m_Windows.showStats))
    {
        auto& stats = App::GetInstance()->GetStats();
        ImGui::Text("FPS: %u", stats.fps.GetFPS());
        ImGui::Text("Frame: %.2f ms", stats.frameTime);
        ImGui::Separator();
        ImGui::TextUnformatted("Frame budget (120-frame averages)");
        // Disjoint top-level App::Run scopes: avoid adding nested physics,
        // sprite, or backend submit scopes a second time.
        struct BudgetSegment
        {
            const char* name;
            FrameProfiler::Zone zone;
            ImU32 color;
        };
        static constexpr BudgetSegment segments[] = {
            {"Input", FrameProfiler::Zone::Input, IM_COL32(105, 175, 245, 255)},
            {"Updates", FrameProfiler::Zone::LayerUpdate, IM_COL32(226, 138, 73, 255)},
            {"Begin", FrameProfiler::Zone::RendererBegin, IM_COL32(157, 129, 227, 255)},
            {"App", FrameProfiler::Zone::AppUpdate, IM_COL32(198, 131, 178, 255)},
            {"Render", FrameProfiler::Zone::LayerRender, IM_COL32(80, 187, 152, 255)},
            {"Editor", FrameProfiler::Zone::EditorUI, IM_COL32(209, 182, 81, 255)},
            {"End", FrameProfiler::Zone::RendererEnd, IM_COL32(140, 149, 164, 255)},
            {"Submit/present", FrameProfiler::Zone::Submit, IM_COL32(217, 103, 103, 255)}
        };
        const float frameAverage = FrameProfiler::Get(FrameProfiler::Zone::Frame).average;
        float tracked = 0.0f;
        for (const auto& segment : segments)
            tracked += FrameProfiler::Get(segment.zone).average;
        const float remaining = std::max(0.0f, frameAverage - tracked);
        const float denominator = std::max(frameAverage, tracked);
        const ImVec2 barStart = ImGui::GetCursorScreenPos();
        const float barWidth = ImGui::GetContentRegionAvail().x;
        const float barHeight = 24.0f;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float cursor = barStart.x;
        if (denominator > 0.0f && barWidth > 0.0f)
        {
            for (const auto& segment : segments)
            {
                const float duration = FrameProfiler::Get(segment.zone).average;
                const float width = barWidth * duration / denominator;
                if (width > 0.0f)
                    drawList->AddRectFilled(ImVec2(cursor, barStart.y),
                        ImVec2(cursor + width, barStart.y + barHeight), segment.color);
                cursor += width;
            }
            const float restWidth = barWidth * remaining / denominator;
            if (restWidth > 0.0f)
                drawList->AddRectFilled(ImVec2(cursor, barStart.y),
                    ImVec2(cursor + restWidth, barStart.y + barHeight),
                    IM_COL32(90, 95, 105, 255));
        }
        ImGui::Dummy(ImVec2(barWidth, barHeight));
        // A wrapping legend stays readable even with a narrow Stats panel.
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        float rowWidth = 0.0f;
        for (const auto& segment : segments)
        {
            const float duration = FrameProfiler::Get(segment.zone).average;
            char label[128];
            std::snprintf(label, sizeof(label), "%s %.2f ms", segment.name, duration);
            const float itemWidth = 14.0f + ImGui::CalcTextSize(label).x + 18.0f;
            if (rowWidth > 0.0f && rowWidth + itemWidth > availableWidth)
            {
                ImGui::NewLine();
                rowWidth = 0.0f;
            }
            else if (rowWidth > 0.0f)
                ImGui::SameLine(0.0f, 10.0f);

            const ImVec2 marker = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(marker.x, marker.y + 3.0f),
                ImVec2(marker.x + 10.0f, marker.y + 13.0f),
                segment.color);
            ImGui::Dummy(ImVec2(12.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine(0.0f, 2.0f);
            ImGui::TextUnformatted(label);
            rowWidth += itemWidth;
        }
        ImGui::TextDisabled("Other / profiling overhead: %.2f ms", remaining);
        ImGui::Separator();
        ImGui::TextUnformatted("CPU profiler (last / avg / peak, 120 frames)");
        if (ImGui::BeginTable("ProfilerTimes", 4,
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Scope");
            ImGui::TableSetupColumn("Last (ms)");
            ImGui::TableSetupColumn("Avg (ms)");
            ImGui::TableSetupColumn("Peak (ms)");
            ImGui::TableHeadersRow();
            for (std::size_t i = 0; i < FrameProfiler::Count; ++i)
            {
                const auto zone = static_cast<FrameProfiler::Zone>(i);
                const auto result = FrameProfiler::Get(zone);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FrameProfiler::Name(zone));
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", result.last);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", result.average);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", result.max);
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::Text("Quads: %i  Calls: %i  Batches: %i",
            stats.rendering.QuadCount, stats.rendering.DrawCalls,
            stats.rendering.BatchCount);
        ImGui::Text("Batch build: %.3f ms", stats.rendering.BatchBuildTime);
        ImGui::TextDisabled("CPU scopes can overlap; GPU time is not included.");
        ImGui::TextDisabled("Backend timings are nested within Draw/present.");
        ImGui::TextDisabled("Current frame is finalized after this UI draw.");
    }
    ImGui::End();
}

void EditorLayer::begin_dock_space()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

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

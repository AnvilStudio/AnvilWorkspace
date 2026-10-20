#include "EditorLayer.h"
#include "MainMenu.h"
#include <algorithm>
#include <cstdio>
#include <initializer_list>

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
    m_ImGuiIniPath = (fs.GetKeyVal("Settings") / "EditorConfig" / "EditorLayout.ini").string();
    io.IniFilename = m_ImGuiIniPath.c_str();
    auto path = fs.GetKeyVal("Assets") / "Fonts" / "JetBrainsMono-Bold.ttf";

    io.Fonts->AddFontFromFileTTF(path.string().c_str(), 18.0f);
    io.ConfigDpiScaleFonts = true;

    ImGuiStyle &style = ImGui::GetStyle();

    // Forge Graphite: calm charcoal surfaces with restrained steel-blue accents.
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.TabRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.WindowPadding = ImVec2(8, 7);
    style.FramePadding = ImVec2(8, 5);
    style.CellPadding = ImVec2(7, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 11.0f;

    ImVec4* colors = style.Colors;
    const ImVec4 background(0.092f, 0.100f, 0.112f, 1.0f);
    const ImVec4 panel(0.123f, 0.132f, 0.146f, 1.0f);
    const ImVec4 raised(0.165f, 0.175f, 0.193f, 1.0f);
    const ImVec4 hover(0.213f, 0.237f, 0.267f, 1.0f);
    const ImVec4 accent(0.337f, 0.569f, 0.773f, 1.0f);
    const ImVec4 accentDim(0.235f, 0.383f, 0.517f, 1.0f);

    colors[ImGuiCol_Text] = ImVec4(0.910f, 0.925f, 0.945f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.485f, 0.510f, 0.552f, 1.0f);
    colors[ImGuiCol_WindowBg] = background;
    colors[ImGuiCol_ChildBg] = panel;
    colors[ImGuiCol_PopupBg] = panel;
    colors[ImGuiCol_Border] = ImVec4(0.255f, 0.275f, 0.307f, 0.85f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = raised;
    colors[ImGuiCol_FrameBgHovered] = hover;
    colors[ImGuiCol_FrameBgActive] = accentDim;
    colors[ImGuiCol_TitleBg] = background;
    colors[ImGuiCol_TitleBgActive] = panel;
    colors[ImGuiCol_TitleBgCollapsed] = background;
    colors[ImGuiCol_MenuBarBg] = panel;
    colors[ImGuiCol_ScrollbarBg] = background;
    colors[ImGuiCol_ScrollbarGrab] = raised;
    colors[ImGuiCol_ScrollbarGrabHovered] = hover;
    colors[ImGuiCol_ScrollbarGrabActive] = accentDim;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.49f, 0.70f, 0.87f, 1.0f);
    colors[ImGuiCol_Button] = raised;
    colors[ImGuiCol_ButtonHovered] = hover;
    colors[ImGuiCol_ButtonActive] = accentDim;
    colors[ImGuiCol_Header] = raised;
    colors[ImGuiCol_HeaderHovered] = hover;
    colors[ImGuiCol_HeaderActive] = accentDim;
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = accentDim;
    colors[ImGuiCol_SeparatorActive] = accent;
    colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.2f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.6f);
    colors[ImGuiCol_ResizeGripActive] = accent;
    colors[ImGuiCol_Tab] = panel;
    colors[ImGuiCol_TabHovered] = hover;
    colors[ImGuiCol_TabActive] = raised;
    colors[ImGuiCol_TabUnfocused] = background;
    colors[ImGuiCol_TabUnfocusedActive] = panel;
    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.65f);
    colors[ImGuiCol_DockingEmptyBg] = background;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_NavHighlight] = accent;
    colors[ImGuiCol_TableHeaderBg] = panel;
    colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_Border];
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.255f, 0.275f, 0.307f, 0.45f);
    colors[ImGuiCol_TableRowBg] = background;
    colors[ImGuiCol_TableRowBgAlt] = panel;

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
    {
        ANV_PROFILE_ZONE(ForgeDockspace);
        begin_dock_space();
    }
    {
        ANV_PROFILE_ZONE(ForgeMainMenu);
        m_MainMenu.Draw();
    }

    auto sceneManager = App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;

    {
        ANV_PROFILE_ZONE(ForgeHierarchy);
        if (m_Windows.showHierarchy)
            m_SceneHierarchy.Draw();
    }
    {
        ANV_PROFILE_ZONE(ForgeFileBrowser);
        if (m_Windows.showFiles)
            m_FileBrowser.Draw();
    }
    {
        ANV_PROFILE_ZONE(ForgeInspector);
        if (m_Windows.showInspector)
            m_InspectorLayer.Draw(scene, m_SelectedEntity);
    }
    {
        ANV_PROFILE_ZONE(ForgeStats);
        draw_stats();
    }
    {
        ANV_PROFILE_ZONE(ForgeNotes);
        m_DevNotes.OnImGuiRender(&m_Windows.showCodeEditor);
    }
    {
        ANV_PROFILE_ZONE(ForgeAssetRegistry);
        m_AssetRegistryPanel.Draw(&m_Windows.showAssetRegistry);
    }
    {
        ANV_PROFILE_ZONE(ForgeConsole);
        m_Console.Draw(&m_Windows.showConsole);
    }
    {
        ANV_PROFILE_ZONE(ForgeGameViewport);
        if (m_Windows.showGameViewport)
            m_GameViewport.Draw();
    }
    {
        ANV_PROFILE_ZONE(ForgeViewport);
        if (m_Windows.showViewport)
            m_Viewport.Draw();
    }
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
        ImGui::Separator();
        ImGui::TextUnformatted("CPU profiler - 120-frame history");
        const auto drawGroup = [](const char* name,
            std::initializer_list<FrameProfiler::Zone> zones, bool defaultOpen)
        {
            ImGui::SetNextItemOpen(defaultOpen, ImGuiCond_Once);
            if (!ImGui::CollapsingHeader(name))
                return;
            ImGui::PushID(name);
            if (ImGui::BeginTable("Times", 4,
                ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Scope");
                ImGui::TableSetupColumn("Last");
                ImGui::TableSetupColumn("Avg");
                ImGui::TableSetupColumn("Peak");
                ImGui::TableHeadersRow();
                for (auto zone : zones)
                {
                    const auto result = FrameProfiler::Get(zone);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImU32 markerColor = 0;
                    for (const auto& segment : segments)
                        if (segment.zone == zone)
                        {
                            markerColor = segment.color;
                            break;
                        }
                    if (markerColor != 0)
                    {
                        const ImVec2 pos = ImGui::GetCursorScreenPos();
                        const float markerSize = 9.0f;
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            ImVec2(pos.x, pos.y + 3.0f),
                            ImVec2(pos.x + markerSize, pos.y + 3.0f + markerSize),
                            markerColor);
                        ImGui::Dummy(ImVec2(markerSize + 3.0f, ImGui::GetTextLineHeight()));
                        ImGui::SameLine(0.0f, 2.0f);
                    }
                    ImGui::TextUnformatted(FrameProfiler::Name(zone));
                    ImGui::TableNextColumn(); ImGui::Text("%.2f", result.last);
                    ImGui::TableNextColumn(); ImGui::Text("%.2f", result.average);
                    ImGui::TableNextColumn(); ImGui::Text("%.2f", result.max);
                }
                ImGui::EndTable();
            }
            ImGui::PopID();
        };
        using Z = FrameProfiler::Zone;
        drawGroup("Frame overview", {Z::Frame, Z::Input, Z::LayerUpdate,
            Z::RendererBegin, Z::AppUpdate, Z::LayerRender,
            Z::EditorUI, Z::RendererEnd, Z::Submit}, true);
        drawGroup("Physics", {Z::Physics, Z::BodyManagement, Z::PhysicsToBox2D,
            Z::PhysicsCapture, Z::Box2D, Z::Box2DToScene, Z::PhysicsFixedSteps}, false);
        drawGroup("Sprites & scripting", {Z::SceneRender, Z::SpriteLookup,
            Z::SpriteTransform, Z::SpriteSubmit, Z::Script}, false);
        drawGroup("Vulkan main thread", {Z::GPUFenceWait, Z::SwapchainAcquire,
            Z::RenderSubmitWait, Z::RenderQueueSwap, Z::RenderWorkerWait,
            Z::ImGuiFinish, Z::Present}, true);
        drawGroup("Vulkan render worker", {Z::WorkerTotal, Z::WorkerBegin,
            Z::WorkerTasks, Z::WorkerEnd, Z::WorkerSubmit,
            Z::SpriteSort, Z::SpriteBuild, Z::SpriteUpload, Z::SpriteCommands}, true);
        drawGroup("Metal", {Z::MetalEncode, Z::MetalCommit}, false);
        drawGroup("Forge editor panels", {Z::ForgeHierarchy, Z::ForgeInspector,
            Z::ForgeFileBrowser, Z::ForgeStats, Z::ForgeNotes,
            Z::ForgeAssetRegistry, Z::ForgeConsole, Z::ForgeGameViewport,
            Z::ForgeViewport, Z::ForgeMainMenu, Z::ForgeDockspace}, true);
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

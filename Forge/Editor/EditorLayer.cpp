#include "EditorLayer.h"
#include "EditorHelpers.h"
// #include "Windows/Viewport.h"

using namespace anv;

EditorLayer::SceneState EditorLayer::m_SceneState = SceneState::Edit;
EditorLayer* EditorLayer::m_This = nullptr;

EditorLayer::EditorLayer()
    : anv::Layer("Editor Layer"),
      m_DevNotes(anv::App::GetInstance()->GetFS().GetKeyVal("Assets") / "Notes.toml"),
      m_FileBrowser(anv::App::GetInstance()->GetFS().GetKeyVal("Assets"))
{
    m_This = this;
}

void EditorLayer::OnAttach()
{
    ImGuiIO &io = ImGui::GetIO();

    auto &fs = App::GetInstance()->GetFS();
    auto path = fs.GetKeyVal("Assets") / "Fonts/JetBrainsMono-Bold.ttf";

    io.Fonts->AddFontFromFileTTF(
        path.string().c_str(),
        18.0f);

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

    // Main backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f); // #1E1E1E
    colors[ImGuiCol_ChildBg] = ImVec4(0.145f, 0.145f, 0.149f, 1.0f);  // #252526
    colors[ImGuiCol_PopupBg] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);  // #2D2D30

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.12f, 0.12f, 0.12f, 0.8f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.831f, 0.831f, 0.831f, 1.0f); // #D4D4D4
    colors[ImGuiCol_TextDisabled] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);        // #3A3D41
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f); // #4A90E2
    colors[ImGuiCol_ButtonActive] = ImVec4(0.208f, 0.478f, 0.741f, 1.0f);  // #357ABD

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.145f, 0.145f, 0.149f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(0.208f, 0.478f, 0.741f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);

    // Title bars
    colors[ImGuiCol_TitleBg] = ImVec4(0.110f, 0.110f, 0.110f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.145f, 0.145f, 0.149f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);

    // Frame backgrounds
    colors[ImGuiCol_FrameBg] = ImVec4(0.176f, 0.176f, 0.188f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);

    // Selection
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.149f, 0.310f, 0.471f, 1.0f); // #264F78

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.290f, 0.565f, 0.886f, 0.7f);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.227f, 0.239f, 0.255f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.290f, 0.565f, 0.886f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.208f, 0.478f, 0.741f, 1.0f);

    anv_log::AnvLog::SetCallback(
        [this](const anv_log::LogRecord &record)
        {
            m_Console.AddLogRecord(record);
        });

    ANV_LOG_INFO(
        "Forge console connected to AnvLog");
}

void EditorLayer::OnUpdate(float dt)
{
}

void EditorLayer::OnImGuiRender()
{
    begin_dock_space();
    draw_menu_bar();
    draw_scene_hierarchy();

    // Draw the source panel before drop targets so a newly-started drag is
    // available to the Sprite Renderer slot and Viewport in the same frame.
    m_FileBrowser.Draw();

    draw_inspector();
    draw_stats();
    m_DevNotes.OnImGuiRender(&m_Windows.showCodeEditor);
    m_AssetRegistryPanel.Draw(&m_Windows.showAssetRegistry);
    m_Console.Draw(&m_Windows.showConsole);
    m_GameViewport.Draw();
    m_Viewport.Draw(); // viewport drawn last so its auto focused on startup
}

void EditorLayer::OnDetach()
{
    anv_log::AnvLog::ClearCallback();
}

void EditorLayer::draw_scene_hierarchy()
{
    ImGui::Begin("Scene Hierarchy");

    // EditorLayer
    auto scene = App::GetInstance()->GetSceneManager()->GetActive();
    if (!scene)
        return;

    if (ImGui::Button("Create Entity"))
    {
        auto entity =
            scene->CreateEntity("New Entity");

        scene->AddComponent<Component::SpriteRenderer>(
            entity);
    }
    ImGui::Separator();
    auto view =
        scene->Registry().view<Component::Tag>();

    for (auto entity : view)
    {
        auto &tag =
            view.get<Component::Tag>(entity);

        bool selected =
            m_SelectedEntity == entity;

        std::string label =
            tag.Get() +
            "##" +
            std::to_string(
                static_cast<uint32_t>(entity));

        if (ImGui::Selectable(
                label.c_str(),
                selected))
        {
            m_SelectedEntity = entity;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                m_EntityToDelete = entity;
            }

            ImGui::EndPopup();
        }
    }

    if (m_EntityToDelete != entt::null)
    {
        const entt::entity deletedEntity = m_EntityToDelete;
        scene->DestroyEntity(deletedEntity);
        m_EntityToDelete = entt::null;
        if (m_SelectedEntity == deletedEntity)
            m_SelectedEntity = entt::null;
    }

    ImGui::End();
}

void EditorLayer::draw_inspector()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::Begin("Inspector");

    auto scene =
        App::GetInstance()
            ->GetSceneManager()
            ->GetActive();

    if (m_SelectedEntity != entt::null &&
        scene->Registry().valid(m_SelectedEntity))
    {

        draw_component<Component::Tag>(
            "Tag",
            m_SelectedEntity,
            scene,
            [&](Component::Tag &tag)
            {
                char buffer[256]{};
                strncpy(buffer, tag.Get().c_str(), sizeof(buffer));

                if (ImGui::BeginTable("TagProps", 2,
                                      ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();

                    if (property_text("Name", buffer, sizeof(buffer)))
                        tag.value = buffer;

                    ImGui::EndTable();
                }
            });

        draw_component<Component::Transform2d>(
            "Transform2D",
            m_SelectedEntity,
            scene,
            [&](Component::Transform2d &transform)
            {
                if (ImGui::BeginTable("Transform2DProps", 2,
                                      ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    property_float2("Position", &transform.position.x);

                    ImGui::TableNextRow();
                    property_float("Rotation", &transform.rotation);

                    ImGui::TableNextRow();
                    property_float2("Scale", &transform.scale.x);

                    ImGui::EndTable();
                }
            });

        draw_component<Component::SpriteRenderer>(
            "Sprite Renderer",
            m_SelectedEntity,
            scene,
            [&](Component::SpriteRenderer &sprite)
            {
                auto assetManager = App::GetInstance()->GetAssetManager();

                Ref<Texture> currentTexture = assetManager
                                                  ? assetManager->GetAs<Texture>(sprite.texture)
                                                  : nullptr;

                if (ImGui::BeginTable(
                        "SpriteProps",
                        2,
                        ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn(
                        "Label",
                        ImGuiTableColumnFlags_WidthFixed,
                        80.0f);

                    ImGui::TableSetupColumn(
                        "Value",
                        ImGuiTableColumnFlags_WidthStretch);

                    // Texture
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Texture");

                    ImGui::TableSetColumnIndex(1);

                    const char *textureLabel = currentTexture
                                                   ? currentTexture->GetName().c_str()
                                                   : "None — drop texture here";

                    ImGui::Button(
                        textureLabel,
                        ImVec2(-1.0f, 0.0f));

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload =
                                ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
                        {
                            const char *pathData =
                                static_cast<const char *>(payload->Data);

                            std::filesystem::path texturePath(pathData ? pathData : "");

                            std::string extension = texturePath.extension().string();
                            std::transform(
                                extension.begin(),
                                extension.end(),
                                extension.begin(),
                                [](unsigned char character)
                                {
                                    return static_cast<char>(std::tolower(character));
                                });

                            const bool isTexture =
                                extension == ".png" ||
                                extension == ".jpg" ||
                                extension == ".jpeg" ||
                                extension == ".bmp" ||
                                extension == ".tga";

                            if (isTexture && assetManager)
                            {
                                Ref<Texture> texture =
                                    assetManager->GetOrCreateTexture(texturePath);

                                if (texture && texture->IsGPUReady())
                                {
                                    sprite.texture = texture->GetAssetID();
                                    currentTexture = texture;
                                    scene->Save();
                                }
                                else
                                {
                                    ANV_LOG_ERROR(
                                        "Failed to assign texture to SpriteRenderer: %s",
                                        texturePath.string().c_str());
                                }
                            }
                        }

                        ImGui::EndDragDropTarget();
                    }

                    if (currentTexture)
                    {
                        ImGui::TextDisabled(
                            "%dx%d | %s",
                            currentTexture->Width(),
                            currentTexture->Height(),
                            currentTexture->IsGPUReady()
                                ? "GPU ready"
                                : "GPU unavailable");

                        if (ImGui::Button("Clear Texture"))
                        {
                            sprite.texture = {};
                            currentTexture = nullptr;
                            scene->Save();
                        }
                    }
                    else if (!sprite.texture.uuid.empty())
                    {
                        ImGui::TextDisabled(
                            "Missing asset: %s",
                            sprite.texture.uuid.c_str());
                    }

                    // Color
                    ImGui::TableNextRow();
                    property_color4("Color", &sprite.color.x);

                    // Draw layer
                    ImGui::TableNextRow();
                    property_int("Draw Layer", &sprite.drawLayer);

                    ImGui::EndTable();
                }
            });
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void EditorLayer::draw_add_component_menu(
    anv::Ref<anv::Scene> scene,
    entt::entity entity)
{
    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (!scene->HasComponent<anv::Component::SpriteRenderer>(entity))
        {
            if (ImGui::MenuItem("Sprite Renderer"))
            {
                scene->AddComponent<anv::Component::SpriteRenderer>(entity);
                ImGui::CloseCurrentPopup();
            }
        }

        if (!scene->HasComponent<anv::Component::Transform2d>(entity))
        {
            if (ImGui::MenuItem("Transform 2D"))
            {
                scene->AddComponent<anv::Component::Transform2d>(entity);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void EditorLayer::draw_stats()
{
    if (!m_Windows.showStats)
        return;

    ImGui::Begin("Stats");

    auto &stats =
        App::GetInstance()->GetStats();

    ImGui::Text(
        "FPS: %u",
        stats.fps.GetFPS());

    ImGui::Text(
        "Frame Time: %.3f ms",
        stats.frameTime);

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

    ImGuiID dockspaceID = ImGui::GetID("AnvilDockspace");

    ImGuiDockNodeFlags dockFlags =
        ImGuiDockNodeFlags_PassthruCentralNode;

    ImGui::DockSpace(
        dockspaceID,
        ImVec2(0.0f, 0.0f),
        dockFlags);

    ImGui::End();
}

void EditorLayer::draw_menu_bar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                auto scene =
                    App::GetInstance()
                        ->GetSceneManager()
                        ->GetActive();

                if (scene)
                    scene->Save();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Reload Scene"))
            {
                App::GetInstance()->GetSceneManager()->ReloadActive();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                App::GetInstance()->Close();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem(
                "Asset Registry",
                nullptr,
                &m_Windows.showAssetRegistry);

            ImGui::MenuItem(
                "Dev Notes",
                nullptr,
                &m_Windows.showCodeEditor);

            ImGui::MenuItem(
                "Stats",
                nullptr,
                &m_Windows.showStats);

            ImGui::MenuItem(
                "Console",
                nullptr,
                &m_Windows.showConsole);

            ImGui::EndMenu();
        }

        ImGui::SetCursorPosX(
            (ImGui::GetWindowWidth() - 80) * 0.5f);

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.18f, 0.45f, 0.22f, 1.0f));

        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.22f, 0.55f, 0.27f, 1.0f));

        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.15f, 0.35f, 0.18f, 1.0f));

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
}
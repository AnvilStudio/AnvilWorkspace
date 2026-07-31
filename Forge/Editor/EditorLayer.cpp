#include "EditorLayer.h"
#include "EditorHelpers.h"

#include <algorithm>

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
    draw_menu_bar();
    m_SceneHierarchy.Draw();
    m_FileBrowser.Draw();
    draw_inspector();
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

void EditorLayer::draw_inspector()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::Begin("Inspector");

    auto scene = App::GetInstance()->GetSceneManager()->GetActive();

    if (scene &&
        m_SelectedEntity != entt::null &&
        scene->Registry().valid(m_SelectedEntity))
    {
        draw_component<Component::Tag>(
            "Tag",
            m_SelectedEntity,
            scene,
            [&](Component::Tag &tag)
            {
                char buffer[256]{};
                std::strncpy(buffer, tag.Get().c_str(), sizeof(buffer) - 1);

                if (ImGui::BeginTable("TagProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
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
                if (ImGui::BeginTable("Transform2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
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

                if (ImGui::BeginTable("SpriteProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Texture");
                    ImGui::TableSetColumnIndex(1);

                    const char *textureLabel = currentTexture
                        ? currentTexture->GetName().c_str()
                        : "None — drop texture here";

                    ImGui::Button(textureLabel, ImVec2(-1.0f, 0.0f));

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
                        {
                            const char *pathData = static_cast<const char *>(payload->Data);
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
                                Ref<Texture> texture = assetManager->GetOrCreateTexture(texturePath);
                                if (texture && texture->IsGPUReady())
                                {
                                    sprite.texture = texture->GetAssetID();
                                    currentTexture = texture;
                                    scene->Save();
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
                            currentTexture->IsGPUReady() ? "GPU ready" : "GPU unavailable");

                        if (ImGui::Button("Clear Texture"))
                        {
                            sprite.texture = {};
                            currentTexture = nullptr;
                            scene->Save();
                        }
                    }
                    else if (!sprite.texture.uuid.empty())
                    {
                        ImGui::TextDisabled("Missing asset: %s", sprite.texture.uuid.c_str());
                    }

                    ImGui::TableNextRow();
                    property_color4("Color", &sprite.color.x);
                    ImGui::TableNextRow();
                    property_int("Draw Layer", &sprite.drawLayer);
                    ImGui::EndTable();
                }
            });

        draw_component<Component::Rigidbody2D>(
            "Rigidbody 2D",
            m_SelectedEntity,
            scene,
            [&](Component::Rigidbody2D &rigidbody)
            {
                if (ImGui::BeginTable("Rigidbody2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    property_label("Body Type");
                    int bodyType = static_cast<int>(rigidbody.type);
                    const char* bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
                    if (ImGui::Combo("##Body Type", &bodyType, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
                    {
                        rigidbody.type = static_cast<Component::Rigidbody2DType>(bodyType);
                    }

                    ImGui::TableNextRow();
                    if (property_float("Gravity Scale", &rigidbody.gravityScale))
                        rigidbody.gravityScale = std::max(0.0f, rigidbody.gravityScale);

                    ImGui::TableNextRow();
                    if (property_float("Linear Damping", &rigidbody.linearDamping))
                        rigidbody.linearDamping = std::max(0.0f, rigidbody.linearDamping);

                    ImGui::TableNextRow();
                    if (property_float("Angular Damping", &rigidbody.angularDamping))
                        rigidbody.angularDamping = std::max(0.0f, rigidbody.angularDamping);

                    ImGui::TableNextRow();
                    property_bool("Fixed Rotation", &rigidbody.fixedRotation);

                    ImGui::TableNextRow();
                    property_bool("Bullet", &rigidbody.bullet);

                    ImGui::TableNextRow();
                    property_bool("Enabled", &rigidbody.enabled);

                    ImGui::EndTable();
                }
            });

        draw_component<Component::BoxCollider2D>(
            "Box Collider 2D",
            m_SelectedEntity,
            scene,
            [&](Component::BoxCollider2D &collider)
            {
                if (ImGui::BeginTable("BoxCollider2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    if (property_float2("Size", &collider.size.x))
                    {
                        collider.size.x = std::max(0.001f, collider.size.x);
                        collider.size.y = std::max(0.001f, collider.size.y);
                    }

                    ImGui::TableNextRow();
                    property_float2("Offset", &collider.offset.x);

                    ImGui::TableNextRow();
                    if (property_float("Density", &collider.density))
                        collider.density = std::max(0.0f, collider.density);

                    ImGui::TableNextRow();
                    if (property_float("Friction", &collider.friction))
                        collider.friction = std::max(0.0f, collider.friction);

                    ImGui::TableNextRow();
                    if (property_float("Restitution", &collider.restitution))
                        collider.restitution = std::clamp(collider.restitution, 0.0f, 1.0f);

                    ImGui::TableNextRow();
                    property_bool("Sensor", &collider.sensor);

                    ImGui::EndTable();
                }
            });

        draw_add_component_menu(scene, m_SelectedEntity);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void EditorLayer::draw_add_component_menu(
    anv::Ref<anv::Scene> scene,
    entt::entity entity)
{
    if (!scene || entity == entt::null || !scene->Registry().valid(entity))
        return;

    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (!scene->HasComponent<Component::SpriteRenderer>(entity) &&
            ImGui::MenuItem("Sprite Renderer"))
        {
            scene->AddComponent<Component::SpriteRenderer>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::Transform2d>(entity) &&
            ImGui::MenuItem("Transform 2D"))
        {
            scene->AddComponent<Component::Transform2d>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::Rigidbody2D>(entity) &&
            ImGui::MenuItem("Rigidbody 2D"))
        {
            scene->AddComponent<Component::Rigidbody2D>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::BoxCollider2D>(entity) &&
            ImGui::MenuItem("Box Collider 2D"))
        {
            scene->AddComponent<Component::BoxCollider2D>(entity);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void EditorLayer::draw_stats()
{
    if (!m_Windows.showStats)
        return;

    ImGui::Begin("Stats");
    auto &stats = App::GetInstance()->GetStats();
    ImGui::Text("FPS: %u", stats.fps.GetFPS());
    ImGui::Text("Frame Time: %.3f ms", stats.frameTime);
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
    ImGui::DockSpace(
        dockspaceID,
        ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_PassthruCentralNode);

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
                auto scene = App::GetInstance()->GetSceneManager()->GetActive();
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

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 80) * 0.5f);

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
}

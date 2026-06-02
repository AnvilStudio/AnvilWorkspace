#include "EditorLayer.h"

using namespace anv;

EditorLayer::EditorLayer()
	: anv::Layer("Editor Layer")
{
}

void EditorLayer::OnUpdate(float dt)
{


}

void EditorLayer::OnImGuiRender()
{
    begin_dock_space();
    draw_menu_bar();
    draw_scene_hierarchy();
    draw_inspector();
    draw_stats();
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
            entity
        );
    }

    auto view =
        scene->Registry().view<Component::Tag>();

    for (auto entity : view)
    {
        auto& tag =
            view.get<Component::Tag>(entity);

        bool selected =
            m_SelectedEntity == entity;

        std::string label =
            tag.Get() +
            "##" +
            std::to_string(
                static_cast<uint32_t>(entity)
            );

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
        scene->DestroyEntity(m_EntityToDelete);
        m_EntityToDelete = entt::null;
        if (m_SelectedEntity == m_EntityToDelete)
            m_SelectedEntity = entt::null;
    }

    ImGui::End();
}

void EditorLayer::draw_inspector()
{
    ImGui::Begin("Inspector");

    auto scene =
        App::GetInstance()
        ->GetSceneManager()
        ->GetActive();

    if (m_SelectedEntity != entt::null &&
        scene->Registry().valid(m_SelectedEntity))
    {
        auto& registry =
            scene->Registry();

        if (registry.any_of<Component::Tag>(
            m_SelectedEntity))
        {
            auto& tag =
                registry.get<Component::Tag>(
                    m_SelectedEntity);

            char buffer[256]{};
            strncpy(buffer, tag.Get().c_str(),
                sizeof(buffer));

            if (ImGui::InputText(
                "Name",
                buffer,
                sizeof(buffer)))
            {
                tag.value = buffer;
            }
        }

        if (registry.any_of<Component::Transform2d>(
            m_SelectedEntity))
        {
            auto& transform =
                registry.get<Component::Transform2d>(
                    m_SelectedEntity);

            ImGui::DragFloat2(
                "Position",
                &transform.position.x,
                0.1f
            );

            ImGui::DragFloat(
                "Rotation",
                &transform.rotation,
                0.1f
            );

            ImGui::DragFloat2(
                "Scale",
                &transform.scale.x,
                0.1f
            );
        }

        if (registry.any_of<Component::SpriteRenderer>(
            m_SelectedEntity))
        {
            auto& sprite =
                registry.get<Component::SpriteRenderer>(
                    m_SelectedEntity);

            ImGui::ColorEdit4(
                "Color",
                &sprite.color.x
            );
        }
    }

    ImGui::End();
}

void EditorLayer::draw_stats()
{
    ImGui::Begin("Stats");

    auto& stats =
        App::GetInstance()->GetStats();

    ImGui::Text(
        "FPS: %u",
        stats.fps.GetFPS()
    );

    ImGui::Text(
        "Frame Time: %.3f ms",
        stats.frameTime
    );

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

    ImGuiID dockspaceID = ImGui::GetID("AnvilDockspace");

    ImGuiDockNodeFlags dockFlags =
        ImGuiDockNodeFlags_PassthruCentralNode;

    ImGui::DockSpace(
        dockspaceID,
        ImVec2(0.0f, 0.0f),
        dockFlags
    );

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

            if (ImGui::MenuItem("Exit"))
            {
                App::GetInstance()->Close();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

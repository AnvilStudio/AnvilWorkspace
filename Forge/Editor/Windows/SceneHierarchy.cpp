#include "SceneHierarchy.h"
#include "../EditorLayer.h"

SceneHierarchy::SceneHierarchy(Ref<Scene> scene)
    : m_ActiveScene(scene)
{
}

void SceneHierarchy::Draw()
{

    auto selected_entity = EditorLayer::GetInstance()->GetSelectedEntity();

    ImGui::Begin("Scene Hierarchy");

    // EditorLayer
    auto scene = App::GetInstance()->GetSceneManager()->GetActive();
    if (!scene)
        return;

    ImGui::TextUnformatted(scene->GetName().c_str());

    // Create Entity
    if (ImGui::BeginPopupContextWindow(
        "SceneHierarchyContextMenu",
        ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems))
    {   
        if (ImGui::MenuItem("Create Empty"))
        {
            auto entity =
                scene->CreateEntity("New Sprite");

            m_IsRenaming = true;
            m_RenamedEntity = entity;
        }

        if (ImGui::MenuItem("Create Sprite"))
        {
            auto entity =
                scene->CreateEntity("New Sprite");

            scene->AddComponent<Component::SpriteRenderer>(
                entity);

            m_IsRenaming = true;
            m_RenamedEntity = entity;
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();
    auto view =
        scene->Registry().view<Component::Tag>();

    for (auto entity : view)
    {
        auto &tag =
            view.get<Component::Tag>(entity);

        bool selected =
            selected_entity == entity;

        std::string label =
            tag.Get() +
            "##" +
            std::to_string(
                static_cast<uint32_t>(entity));

        handle_rename(entity, selected_entity, tag, label, selected);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                m_EntityToDelete = entity;
            }

            if (ImGui::MenuItem("Rename"))
            {
                m_RenamedEntity = entity;
                m_IsRenaming = true;

                tag.value = std::string(m_RenameBuffer.data());
            }

            ImGui::EndPopup();
        }
    }

    if (m_EntityToDelete != entt::null)
    {
        const entt::entity deletedEntity = m_EntityToDelete;
        scene->DestroyEntity(deletedEntity);
        m_EntityToDelete = entt::null;
        if (selected_entity == deletedEntity)
            selected_entity = entt::null;
    }

    ImGui::End();
}

void SceneHierarchy::handle_rename(
    entt::entity entity,
    entt::entity& selected_entity,
    Component::Tag &tag,
    std::string label,
    bool selected)
{
    m_RenameBuffer.clear();
    m_RenameBuffer.push_back('\0');

    if (m_IsRenaming && entity == m_RenamedEntity)
    {
        ImGui::SetNextItemWidth(-1);

        ImGui::InputText(
            "##Rename",
            m_RenameBuffer.data(),
            sizeof(m_RenameBuffer.data()),
            ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            tag.value = m_RenameBuffer.data();
            m_IsRenaming = false;
        }

    }
    else
    {
        if (ImGui::Selectable(
                label.c_str(),
                selected))
        {
            selected_entity = entity;
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
        }
    }
}

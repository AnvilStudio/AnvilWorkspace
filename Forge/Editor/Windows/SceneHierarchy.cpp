#include "SceneHierarchy.h"
#include "../EditorLayer.h"

#include <cstring>
#include <vector>
#include <cstdint>

SceneHierarchy::SceneHierarchy(Ref<Scene> scene)
{
    (void)scene;
}

void SceneHierarchy::Draw()
{
    ImGui::Begin("Scene Hierarchy");

    auto scene = App::GetInstance()->GetSceneManager()->GetActive();
    if (!scene)
    {
        ImGui::TextDisabled("No active scene");
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(scene->GetName().c_str());

    if (ImGui::BeginPopupContextWindow(
            "SceneHierarchyContextMenu",
            ImGuiPopupFlags_MouseButtonRight |
                ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            const auto entity = scene->CreateEntity("New Entity");
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
            begin_rename(entity, "New Entity");
        }

        if (ImGui::MenuItem("Create Camera 2D"))
        {
            const auto entity = scene->CreateEntity("Game Camera");
            scene->AddComponent<Component::Camera2D>(entity);

            if (scene->GetActiveCameraEntity() == entt::null)
                scene->SetActiveCamera(entity);

            scene->Save();
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
            begin_rename(entity, "Game Camera");
        }

        if (ImGui::MenuItem("Create Sprite"))
        {
            const auto entity = scene->CreateEntity("New Sprite");
            scene->AddComponent<Component::SpriteRenderer>(entity);
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
            begin_rename(entity, "New Sprite");
        }

        if (ImGui::MenuItem("Create Dynamic Physics Sprite"))
        {
            const auto entity = scene->CreateEntity("Dynamic Body");
            scene->AddComponent<Component::SpriteRenderer>(entity);

            auto& body = scene->AddComponent<Component::Rigidbody2D>(entity);
            body.type = Component::Rigidbody2DType::Dynamic;

            scene->AddComponent<Component::BoxCollider2D>(entity);
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
            begin_rename(entity, "Dynamic Body");
        }

        if (ImGui::MenuItem("Create Static Barrier"))
        {
            const auto entity = scene->CreateEntity("Static Barrier");

            auto& body = scene->AddComponent<Component::Rigidbody2D>(entity);
            body.type = Component::Rigidbody2DType::Static;

            scene->AddComponent<Component::BoxCollider2D>(entity);
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
            begin_rename(entity, "Static Barrier");
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    const auto selectedEntity =
        EditorLayer::GetInstance()->GetSelectedEntity();

    // Only roots appear at top level; descendants render recursively.
    auto view = scene->Registry().view<Component::Tag>();
    for (const auto entity : view)
        if (scene->GetParent(entity) == entt::null)
            draw_entity(scene, entity, selectedEntity, 0);

    // Dropping on empty hierarchy space makes an entity a root.
    ImGui::Dummy(ImVec2(-1.0f, 12.0f));
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("ANV_HIERARCHY_ENTITY"))
        {
            if (payload->DataSize == sizeof(entt::entity))
            {
                const auto dragged = *static_cast<const entt::entity*>(payload->Data);
                if (scene->SetParent(dragged, entt::null)) scene->Save();
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (m_EntityToDelete != entt::null)
    {
        const entt::entity deletedEntity = m_EntityToDelete;
        scene->DestroyEntity(deletedEntity);
        scene->Save();
        m_EntityToDelete = entt::null;

        if (selectedEntity == deletedEntity)
            EditorLayer::GetInstance()->ClearSelection();

        if (m_RenamedEntity == deletedEntity)
            cancel_rename();
    }

    ImGui::End();
}

void SceneHierarchy::draw_entity(Ref<Scene> scene,
                                 entt::entity entity,
                                 entt::entity selected,
                                 int depth)
{
    if (depth > 256 || !scene->Registry().valid(entity) ||
        !scene->HasComponent<Component::Tag>(entity)) return;

    auto& tag = scene->GetComponent<Component::Tag>(entity);
    const auto children = scene->GetChildren(entity);
    ImGui::PushID(static_cast<uint32_t>(entity));
    const bool renaming = m_IsRenaming && m_RenamedEntity == entity;
    const bool hasChildren = !children.empty();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    if (entity == selected) flags |= ImGuiTreeNodeFlags_Selected;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf |
                               ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool open = false;
    if (renaming)
    {
        if (hasChildren) ImGui::SetNextItemOpen(true);
        open = ImGui::TreeNodeEx("##Entity", flags, "%s", "");
        ImGui::SameLine();
        handle_rename(entity, tag, entity == selected);
    }
    else
    {
        open = ImGui::TreeNodeEx("##Entity", flags, "%s", tag.Get().c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
    }

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("ANV_HIERARCHY_ENTITY", &entity, sizeof(entity));
        ImGui::TextUnformatted(tag.Get().c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("ANV_HIERARCHY_ENTITY"))
        {
            if (payload->DataSize == sizeof(entt::entity))
            {
                const auto dragged = *static_cast<const entt::entity*>(payload->Data);
                if (scene->SetParent(dragged, entity)) scene->Save();
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Rename"))
        {
            EditorLayer::GetInstance()->SetSelectedEntity(entity);
            begin_rename(entity, tag.Get());
        }
        if (scene->GetParent(entity) != entt::null && ImGui::MenuItem("Unparent"))
            if (scene->SetParent(entity, entt::null)) scene->Save();
        if (ImGui::MenuItem("Create Child"))
        {
            const auto child = scene->CreateEntity("New Entity");
            scene->SetParent(child, entity, false);
            EditorLayer::GetInstance()->SetSelectedEntity(child);
            begin_rename(child, "New Entity");
            scene->Save();
        }
        if (!scene->HasComponent<Component::Camera2D>(entity) &&
            ImGui::MenuItem("Add Camera 2D"))
        {
            scene->AddComponent<Component::Camera2D>(entity);
            if (scene->GetActiveCameraEntity() == entt::null)
                scene->SetActiveCamera(entity);
            scene->Save();
        }

        if (scene->HasComponent<Component::Camera2D>(entity) &&
            scene->GetActiveCameraEntity() != entity &&
            ImGui::MenuItem("Set As Active Camera"))
        {
            scene->SetActiveCamera(entity);
            scene->Save();
        }

        if (ImGui::BeginMenu("Physics"))
        {
            if (!scene->HasComponent<Component::Rigidbody2D>(entity))
            {
                if (ImGui::MenuItem("Add Dynamic Rigidbody2D"))
                {
                    auto& body = scene->AddComponent<Component::Rigidbody2D>(entity);
                    body.type = Component::Rigidbody2DType::Dynamic;
                }

                if (ImGui::MenuItem("Add Static Rigidbody2D"))
                {
                    auto& body = scene->AddComponent<Component::Rigidbody2D>(entity);
                    body.type = Component::Rigidbody2DType::Static;
                }
            }

            if (!scene->HasComponent<Component::BoxCollider2D>(entity) &&
                ImGui::MenuItem("Add BoxCollider2D"))
            {
                scene->AddComponent<Component::BoxCollider2D>(entity);
            }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Delete")) m_EntityToDelete = entity;
        ImGui::EndPopup();
    }

    if (hasChildren && open)
    {
        for (auto child : children)
            draw_entity(scene, child, selected, depth + 1);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void SceneHierarchy::begin_rename(
    entt::entity entity,
    const std::string& currentName)
{
    m_RenameBuffer.fill('\0');
    std::strncpy(
        m_RenameBuffer.data(),
        currentName.c_str(),
        m_RenameBuffer.size() - 1);

    m_RenameBuffer.back() = '\0';
    m_RenamedEntity = entity;
    m_IsRenaming = true;
    m_FocusRenameInput = true;
}

void SceneHierarchy::handle_rename(
    entt::entity entity,
    Component::Tag& tag,
    bool selected)
{
    if (m_IsRenaming && entity == m_RenamedEntity)
    {
        ImGui::SetNextItemWidth(-1.0f);

        if (m_FocusRenameInput)
        {
            ImGui::SetKeyboardFocusHere();
            m_FocusRenameInput = false;
        }

        const bool submitted = ImGui::InputText(
            "##Rename",
            m_RenameBuffer.data(),
            m_RenameBuffer.size(),
            ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll);

        if (submitted)
        {
            finish_rename(tag);
            return;
        }

        if (ImGui::IsItemActive() &&
            ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            cancel_rename();
            return;
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
            finish_rename(tag);

        return;
    }

    const std::string label = tag.Get() + "##Entity";
    if (ImGui::Selectable(label.c_str(), selected))
        EditorLayer::GetInstance()->SetSelectedEntity(entity);
}

void SceneHierarchy::finish_rename(Component::Tag& tag)
{
    if (m_RenameBuffer[0] != '\0')
        tag.value = m_RenameBuffer.data();

    auto scene = App::GetInstance()->GetSceneManager()->GetActive();
    if (scene) scene->Save();
    cancel_rename();
}

void SceneHierarchy::cancel_rename()
{
    m_RenameBuffer.fill('\0');
    m_RenamedEntity = entt::null;
    m_IsRenaming = false;
    m_FocusRenameInput = false;
}

#include "SceneHierarchy.h"
#include "../EditorLayer.h"

#include <cstring>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>

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
        if (ImGui::MenuItem("Paste", nullptr, false, !m_Clipboard.Empty()))
            paste_entities(scene);
        if (ImGui::MenuItem("Import Prefab..."))
            m_OpenPrefabLoad = true;
        if (ImGui::MenuItem("Refresh All Prefab Instances"))
        {
            std::vector<entt::entity> instances;
            auto refs = scene->Registry().view<Component::PrefabInstance>();
            for (auto instance : refs) instances.push_back(instance);
            for (auto instance : instances)
            {
                if (!scene->Registry().valid(instance)) continue;
                forge::EntityPrefab prefab;
                const auto source = scene->GetComponent<Component::PrefabInstance>(instance).sourcePath;
                if (prefab.Load(source)) prefab.Refresh(scene, instance);
            }
        }
        ImGui::Separator();

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

    // Entity clipboard shortcuts only when the hierarchy owns keyboard focus.
    // Avoid intercepting typing in rename/text fields.
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        !ImGui::GetIO().WantTextInput &&
        (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_C, false))
            copy_entities(scene, EditorLayer::GetInstance()->GetSelectedEntity());
        if (ImGui::IsKeyPressed(ImGuiKey_V, false))
            paste_entities(scene);
        if (ImGui::IsKeyPressed(ImGuiKey_D, false))
        {
            copy_entities(scene, EditorLayer::GetInstance()->GetSelectedEntity());
            paste_entities(scene);
        }
    }

    const auto selectedEntity =
        EditorLayer::GetInstance()->GetSelectedEntity();

    // Build a single adjacency index per frame instead of scanning for every row.
    ChildMap children;
    std::unordered_map<std::string, entt::entity> byUUID;
    auto ids = scene->Registry().view<uuid::EntityUUID>();
    byUUID.reserve(scene->Registry().storage<uuid::EntityUUID>().size());
    for (auto entity : ids)
        byUUID.emplace(ids.get<uuid::EntityUUID>(entity).uuid, entity);

    auto relationships = scene->Registry().view<Component::Relationship>();
    for (auto entity : relationships)
    {
        const auto& parentID = relationships.get<Component::Relationship>(entity).parentUUID;
        const auto parent = byUUID.find(parentID);
        if (parent != byUUID.end())
            children[parent->second].push_back(entity);
    }

    auto view = scene->Registry().view<Component::Tag>();
    std::vector<entt::entity> roots;
    for (const auto entity : view)
        if (!scene->HasComponent<Component::Relationship>(entity) ||
            !byUUID.contains(scene->GetComponent<Component::Relationship>(entity).parentUUID))
            roots.push_back(entity);

    m_VisibleEntities.clear();
    for (auto entity : roots)
        collect_visible(entity, children, 0);
    for (auto entity : roots)
        draw_entity(scene, entity, selectedEntity, 0, children);

    // Accept prefab files dragged from the File Browser onto empty hierarchy space.
    ImGui::Dummy(ImVec2(-1.0f, 12.0f));
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
        {
            if (payload->Data && payload->DataSize > 1 &&
                static_cast<const char*>(payload->Data)[payload->DataSize-1] == '\0')
            {
                const std::filesystem::path path(static_cast<const char*>(payload->Data));
                forge::EntityPrefab prefab;
                if (prefab.Load(path))
                {
                    auto roots = prefab.Instantiate(scene, path);
                    if (!roots.empty()) EditorLayer::GetInstance()->SetSelectedEntities(roots);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Dropping on empty hierarchy space makes an entity a root.
    ImGui::Dummy(ImVec2(-1.0f, 12.0f));
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("ANV_HIERARCHY_ENTITIES"))
        {
            if (payload->DataSize > 0 && payload->DataSize % sizeof(entt::entity) == 0)
            {
                auto* data = static_cast<const entt::entity*>(payload->Data);
                std::vector<entt::entity> entities(data, data + payload->DataSize / sizeof(entt::entity));
                if (reparent_selection(scene, entities, entt::null)) scene->Save();
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

        EditorLayer::GetInstance()->RemoveFromSelection(deletedEntity);

        if (m_RenamedEntity == deletedEntity)
            cancel_rename();
    }

    draw_prefab_dialogs(scene);
    ImGui::End();
}

void SceneHierarchy::draw_entity(Ref<Scene> scene,
                                 entt::entity entity,
                                 entt::entity selected,
                                 int depth, const ChildMap& children)
{
    if (depth > 256 || !scene->Registry().valid(entity) ||
        !scene->HasComponent<Component::Tag>(entity)) return;

    auto& tag = scene->GetComponent<Component::Tag>(entity);
    const auto foundChildren = children.find(entity);
    ImGui::PushID(static_cast<uint32_t>(entity));
    const bool renaming = m_IsRenaming && m_RenamedEntity == entity;
    const bool hasChildren = foundChildren != children.end() && !foundChildren->second.empty();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    if (EditorLayer::GetInstance()->IsEntitySelected(entity)) flags |= ImGuiTreeNodeFlags_Selected;
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
        // Select on release, not mouse-down: mouse-down on an existing
        // multi-selection must not collapse it before drag/drop starts.
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            !ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            !ImGui::IsItemToggledOpen())
            apply_selection(entity);
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        auto* editor = EditorLayer::GetInstance();
        // Preserve every selected entity when dragging any selected row.
        // Dragging a row outside the selection moves only that row.
        std::vector<entt::entity> dragEntities;
        if (editor->IsEntitySelected(entity))
            dragEntities = editor->GetSelectedEntities();
        else
            dragEntities.push_back(entity);
        ImGui::SetDragDropPayload("ANV_HIERARCHY_ENTITIES",
            dragEntities.data(), dragEntities.size() * sizeof(entt::entity));
        if (dragEntities.size() == 1)
            ImGui::Text("Move %s", tag.Get().c_str());
        else
            ImGui::Text("Move %zu selected entities", dragEntities.size());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("ANV_HIERARCHY_ENTITIES"))
        {
            if (payload->DataSize > 0 && payload->DataSize % sizeof(entt::entity) == 0)
            {
                auto* data = static_cast<const entt::entity*>(payload->Data);
                std::vector<entt::entity> entities(data, data + payload->DataSize / sizeof(entt::entity));
                if (reparent_selection(scene, entities, entity)) scene->Save();
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Copy"))
            copy_entities(scene, entity);
        if (ImGui::MenuItem("Paste", nullptr, false, !m_Clipboard.Empty()))
            paste_entities(scene);
        if (ImGui::MenuItem("Duplicate"))
        {
            copy_entities(scene, entity);
            paste_entities(scene);
        }
        if (scene->HasComponent<Component::PrefabInstance>(entity))
        {
            if (ImGui::MenuItem("Refresh From Prefab"))
            {
                forge::EntityPrefab prefab;
                const auto source = scene->GetComponent<Component::PrefabInstance>(entity).sourcePath;
                if (prefab.Load(source)) prefab.Refresh(scene, entity);
            }
            if (ImGui::MenuItem("Unlink Prefab"))
            {
                scene->RemoveComponent<Component::PrefabInstance>(entity);
                scene->Save();
            }
        }
        if (ImGui::MenuItem("Save as Prefab..."))
        {
            copy_entities(scene, entity);
            m_OpenPrefabSave = !m_Clipboard.Empty();
        }
        ImGui::Separator();
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
        for (auto child : foundChildren->second)
            draw_entity(scene, child, selected, depth + 1, children);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void SceneHierarchy::collect_visible(entt::entity entity, const ChildMap& children, int depth)
{
    if (depth > 256) return;
    m_VisibleEntities.push_back(entity);
    auto it = children.find(entity);
    if (it == children.end()) return;
    for (auto child : it->second)
        collect_visible(child, children, depth + 1);
}

void SceneHierarchy::apply_selection(entt::entity entity)
{
    auto* editor = EditorLayer::GetInstance();
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyShift && m_SelectionAnchor != entt::null)
    {
        const auto a = std::find(m_VisibleEntities.begin(), m_VisibleEntities.end(), m_SelectionAnchor);
        const auto b = std::find(m_VisibleEntities.begin(), m_VisibleEntities.end(), entity);
        if (a != m_VisibleEntities.end() && b != m_VisibleEntities.end())
        {
            auto ia = std::distance(m_VisibleEntities.begin(), a);
            auto ib = std::distance(m_VisibleEntities.begin(), b);
            if (ia > ib) std::swap(ia, ib);
            editor->SetSelectedEntities({m_VisibleEntities.begin() + ia,
                m_VisibleEntities.begin() + ib + 1});
            return;
        }
    }
    if (io.KeyCtrl || io.KeySuper)
        editor->ToggleSelectedEntity(entity);
    else
        editor->SetSelectedEntity(entity);
    m_SelectionAnchor = entity;
}

bool SceneHierarchy::reparent_selection(Ref<Scene> scene,
    const std::vector<entt::entity>& selection, entt::entity parent)
{
    if (!scene || (parent != entt::null && !scene->Registry().valid(parent)))
        return false;
    std::vector<entt::entity> roots;
    for (auto entity : selection)
    {
        if (!scene->Registry().valid(entity) || entity == parent ||
            (parent != entt::null && scene->IsDescendant(parent, entity)))
            return false;
        // Moving a selected ancestor implicitly moves its descendants.
        bool nested = false;
        for (auto other : selection)
            if (entity != other && scene->IsDescendant(entity, other))
            { nested = true; break; }
        if (!nested) roots.push_back(entity);
    }
    bool changed = false;
    for (auto entity : roots)
        if (scene->GetParent(entity) != parent &&
            scene->SetParent(entity, parent, true))
            changed = true;
    return changed;
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

std::filesystem::path SceneHierarchy::prefab_path() const
{
    const std::filesystem::path input(m_PrefabPath);
    if (input.empty()) return {};
    if (input.is_absolute()) return input.lexically_normal();
    const auto assets = App::GetInstance()->GetFS().GetKeyVal("Assets");
    const auto it = input.begin();
    if (it != input.end() && it->string() == "Assets")
        return (assets / input.lexically_relative("Assets")).lexically_normal();
    return (assets / input).lexically_normal();
}

void SceneHierarchy::copy_entities(Ref<Scene> scene, entt::entity fallback)
{
    auto* editor = EditorLayer::GetInstance();
    auto selection = editor->GetSelectedEntities();
    if (fallback != entt::null &&
        std::find(selection.begin(), selection.end(), fallback) == selection.end())
        selection = {fallback};
    m_Clipboard.Capture(scene, selection);
}

void SceneHierarchy::paste_entities(Ref<Scene> scene)
{
    if (m_Clipboard.Empty()) return;
    auto roots = m_Clipboard.Instantiate(scene);
    if (!roots.empty()) EditorLayer::GetInstance()->SetSelectedEntities(roots);
}

void SceneHierarchy::draw_prefab_dialogs(Ref<Scene> scene)
{
    if (m_OpenPrefabSave)
    {
        ImGui::OpenPopup("Save Entity Prefab");
        m_OpenPrefabSave = false;
        m_PrefabStatus.clear();
    }
    if (m_OpenPrefabLoad)
    {
        ImGui::OpenPopup("Import Entity Prefab");
        m_OpenPrefabLoad = false;
        m_PrefabStatus.clear();
    }
    auto dialog = [&](const char* title, bool saving)
    {
        if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;
        ImGui::TextUnformatted("Path relative to project Assets:");
        ImGui::SetNextItemWidth(380.0f);
        ImGui::InputText("##PrefabPath", m_PrefabPath, sizeof(m_PrefabPath));
        if (!m_PrefabStatus.empty())
            ImGui::TextWrapped("%s", m_PrefabStatus.c_str());
        if (ImGui::Button(saving ? "Save Prefab" : "Instantiate", ImVec2(130, 0)))
        {
            auto path = prefab_path();
            if (path.extension().empty()) path += ".aprefab";
            if (path.extension() != ".aprefab")
                m_PrefabStatus = "Prefab files must use the .aprefab extension.";
            else if (saving)
            {
                if (m_Clipboard.Save(path))
                {
                    m_PrefabStatus.clear();
                    ImGui::CloseCurrentPopup();
                }
                else m_PrefabStatus = "Failed to write prefab.";
            }
            else
            {
                forge::EntityPrefab prefab;
                if (prefab.Load(path))
                {
                    auto roots = prefab.Instantiate(scene);
                    if (!roots.empty())
                        EditorLayer::GetInstance()->SetSelectedEntities(roots);
                    m_PrefabStatus.clear();
                    ImGui::CloseCurrentPopup();
                }
                else m_PrefabStatus = "Failed to load prefab.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(130, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    };
    dialog("Save Entity Prefab", true);
    dialog("Import Entity Prefab", false);
}

#pragma once

#include <Anvil.h>
#include "EntityPrefab.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

using namespace anv;

class SceneHierarchy
{
public:
    explicit SceneHierarchy(Ref<Scene> scene);

    void Draw();

private:
    using ChildMap = std::unordered_map<entt::entity, std::vector<entt::entity>>;
    void draw_entity(Ref<Scene> scene, entt::entity entity,
                     entt::entity selected, int depth, const ChildMap& children);
    void apply_selection(entt::entity entity);
    void collect_visible(entt::entity entity, const ChildMap& children, int depth);
    bool reparent_selection(Ref<Scene> scene, const std::vector<entt::entity>& selection, entt::entity parent);
    void begin_rename(
        entt::entity entity,
        const std::string& currentName
    );

    void handle_rename(
        entt::entity entity,
        Component::Tag& tag,
        bool selected
    );

    void finish_rename(Component::Tag& tag);
    void cancel_rename();

private:
    forge::EntityPrefab m_Clipboard;
    char m_PrefabPath[512] = "Prefabs/NewPrefab.aprefab";
    bool m_OpenPrefabSave = false;
    bool m_OpenPrefabLoad = false;
    std::string m_PrefabStatus;
    std::filesystem::path prefab_path() const;
    void draw_prefab_dialogs(Ref<Scene> scene);
    void copy_entities(Ref<Scene> scene, entt::entity fallback);
    void paste_entities(Ref<Scene> scene);

    entt::entity m_EntityToDelete = entt::null;
    entt::entity m_RenamedEntity = entt::null;

    bool m_IsRenaming = false;
    bool m_FocusRenameInput = false;

    std::array<char, 256> m_RenameBuffer{};
    std::vector<entt::entity> m_VisibleEntities;
    entt::entity m_SelectionAnchor = entt::null;
};

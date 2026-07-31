#pragma once
#include <Anvil.h>

using namespace anv;

class SceneHierarchy
{
public:
    SceneHierarchy(Ref<Scene> scene);

    void Draw();

private:
    void handle_rename(entt::entity entity, 
    entt::entity& selected_entity,
    Component::Tag& tag, 
    std::string label, 
    bool selected);

private:
    Ref<Scene> m_ActiveScene;
    entt::entity m_EntityToDelete;

    bool m_IsRenaming;
    entt::entity m_RenamedEntity;
    _vec<char> m_RenameBuffer{};
};
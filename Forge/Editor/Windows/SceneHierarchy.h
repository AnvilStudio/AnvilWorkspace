#pragma once

#include <Anvil.h>

#include <array>
#include <string>

using namespace anv;

class SceneHierarchy
{
public:
    explicit SceneHierarchy(Ref<Scene> scene);

    void Draw();

private:
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
    entt::entity m_EntityToDelete = entt::null;
    entt::entity m_RenamedEntity = entt::null;

    bool m_IsRenaming = false;
    bool m_FocusRenameInput = false;

    std::array<char, 256> m_RenameBuffer{};
};

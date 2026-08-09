#pragma once

#include <entt/entt.hpp>

#include <string_view>

namespace anv
{
    class Scene;

    struct ScriptEntityContext
    {
        Scene* scene = nullptr;
        entt::entity entity = entt::null;

        explicit operator bool() const
        {
            return scene != nullptr && entity != entt::null;
        }
    };

    void RegisterScriptScene(Scene& scene);
    void UnregisterScriptScene(Scene& scene);
    void ClearScriptScenes();

    ScriptEntityContext ResolveScriptEntity(std::string_view entityID);
    ScriptEntityContext ResolveScriptEntityByName(std::string_view entityName);
}

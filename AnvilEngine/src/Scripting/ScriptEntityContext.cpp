#include "ScriptEntityContext.h"

#include "../Scene/Component.h"
#include "../Scene/Scene.h"

#include <unordered_set>

namespace anv
{
    namespace
    {
        // Intentional process-lifetime storage: a Scene can be destroyed
        // by static/global reference cleanup after other globals are gone.
        // Keep this registry available for late UnregisterScriptScene calls.
        std::unordered_set<Scene*>& ActiveScriptScenes()
        {
            static auto* scenes = new std::unordered_set<Scene*>();
            return *scenes;
        }
    }

    void RegisterScriptScene(Scene& scene)
    {
        ActiveScriptScenes().insert(&scene);
    }

    void UnregisterScriptScene(Scene& scene)
    {
        ActiveScriptScenes().erase(&scene);
    }

    void ClearScriptScenes()
    {
        ActiveScriptScenes().clear();
    }

    ScriptEntityContext ResolveScriptEntity(std::string_view entityID)
    {
        if (entityID.empty())
            return {};

        for (Scene* scene : ActiveScriptScenes())
        {
            if (!scene)
                continue;

            auto view = scene->Registry().view<uuid::EntityUUID>();
            for (auto [entity, id] : view.each())
            {
                if (std::string_view(id.uuid) == entityID)
                    return {scene, entity};
            }
        }

        return {};
    }

    ScriptEntityContext ResolveScriptEntityByName(std::string_view entityName)
    {
        if (entityName.empty())
            return {};

        for (Scene* scene : ActiveScriptScenes())
        {
            if (!scene)
                continue;

            auto view = scene->Registry().view<Component::Tag, uuid::EntityUUID>();
            for (auto [entity, tag, id] : view.each())
            {
                if (std::string_view(tag.value) == entityName)
                    return {scene, entity};
            }
        }

        return {};
    }
}

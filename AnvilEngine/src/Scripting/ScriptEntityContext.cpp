#include "ScriptEntityContext.h"

#include "../Scene/Component.h"
#include "../Scene/Scene.h"

#include <unordered_set>

namespace anv
{
    namespace
    {
        std::unordered_set<Scene*> s_ActiveScriptScenes;
    }

    void RegisterScriptScene(Scene& scene)
    {
        s_ActiveScriptScenes.insert(&scene);
    }

    void UnregisterScriptScene(Scene& scene)
    {
        s_ActiveScriptScenes.erase(&scene);
    }

    void ClearScriptScenes()
    {
        s_ActiveScriptScenes.clear();
    }

    ScriptEntityContext ResolveScriptEntity(std::string_view entityID)
    {
        if (entityID.empty())
            return {};

        for (Scene* scene : s_ActiveScriptScenes)
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
}

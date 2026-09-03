#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "../vendor/entt/single_include/entt/entt.hpp"

namespace anv
{
    class Scene;

    /**
     * @brief Embedded CPython runtime for entity-attached script instances.
     *
     * Python classes derive from anvil.Script and may implement on_create,
     * on_update(delta_time), and on_destroy. Each entity with Component::Script
     * owns a separate Python object. Annotated bool, int, float, and str fields
     * are reflected into Component::Script::fields and survive scene saves and
     * module reloads.
     *
     * All calls currently execute on Anvil's main thread. Python objects must
     * not be accessed from renderer or worker threads.
     */
    class PythonScriptEngine
    {
    public:
        static bool Initialize(const std::filesystem::path& _projectDirectory);
        static void Shutdown();
        static bool IsInitialized();

        /** Creates missing instances, hot reloads changed modules, and updates scripts. */
        static void UpdateScene(Scene& _scene, float _deltaTime);

        /** Releases all Python instances belonging to a scene. */
        static void ShutdownScene(Scene& _scene);

        /** Calls on_destroy and releases one entity's Python instance. */
        static void DestroyEntity(Scene& _scene, entt::entity _entity);

        /** Forces a module reload on the next scene update. */
        static void RequestReload(const std::filesystem::path& _modulePath = {});

    private:
        static bool create_instance(Scene& _scene, entt::entity _entity);
        static void destroy_instance(const std::string& _entityID, bool _invokeDestroy);
        static void reload_changed_modules(Scene& _scene);
        static void log_python_exception(const char* _context);
    };
}

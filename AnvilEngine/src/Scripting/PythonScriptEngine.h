#pragma once

#include <filesystem>

namespace anv
{
    /**
     * @brief Owns Anvil's embedded CPython interpreter and project script lifecycle.
     *
     * The first scripting milestone loads `Scripts/main.py` from the active
     * project. That module may define `on_create()`, `on_update(delta_time)`,
     * and `on_destroy()` callbacks. Missing callbacks are ignored.
     *
     * Python support is compiled when ANV_ENABLE_PYTHON is defined. Builds that
     * do not provide an embedded Python development environment retain a safe
     * no-op implementation.
     */
    class PythonScriptEngine
    {
    public:
        /**
         * @brief Starts CPython and loads the active project's main script.
         * @param _projectDirectory Root directory of the active Anvil project.
         * @return True when the interpreter initialized successfully.
         */
        static bool Initialize(const std::filesystem::path& _projectDirectory);

        /** Calls the optional Python `on_update(delta_time)` callback. */
        static void Update(float _deltaTime);

        /** Calls `on_destroy()`, releases script objects, and stops CPython. */
        static void Shutdown();

        /** @return True while the embedded interpreter is available. */
        static bool IsInitialized();

    private:
        static bool load_main_module();
        static bool call_no_argument_callback(const char* _callbackName);
        static void log_python_exception(const char* _context);
    };
}

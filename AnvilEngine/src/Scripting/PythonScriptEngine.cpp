#include "PythonScriptEngine.h"

#include "../Core/App.h"
#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../Util/UMacros.h"

#include "../Util/FileSys/FileSystem.h"

#ifdef ANV_ENABLE_PYTHON
#include <Python.h>
#endif

#include <algorithm>
#include <charconv>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

namespace anv
{
    namespace
    {
        bool s_Initialized = false;
        bool s_ReloadAll = false;
        std::filesystem::path s_ProjectDirectory;
        std::filesystem::path s_ScriptsDirectory;
        std::filesystem::path s_RequestedReloadPath;
        std::filesystem::path s_PythonModuleDirectory;
        std::unordered_set<Scene *> s_ActiveScenes;

#ifdef ANV_ENABLE_PYTHON
        struct ModuleRecord
        {
            PyObject *module = nullptr;
            std::filesystem::file_time_type lastWriteTime{};
            std::uint64_t generation = 0;
        };

        struct RuntimeInstance
        {
            Scene *scene = nullptr;
            entt::entity entity = entt::null;
            std::filesystem::path modulePath;
            PyObject *object = nullptr;
        };

        std::unordered_map<std::string, ModuleRecord> s_Modules;
        std::unordered_map<std::string, RuntimeInstance> s_Instances;

        std::string normalize_path(const std::filesystem::path &_path)
        {
            std::error_code error;
            const auto absolute = std::filesystem::weakly_canonical(_path, error);
            return (error ? _path.lexically_normal() : absolute).string();
        }

        std::string module_name(const std::filesystem::path &_path, std::uint64_t _generation)
        {
            const std::size_t hash = std::hash<std::string>{}(normalize_path(_path));
            return "anvil_script_" + std::to_string(hash) + "_" + std::to_string(_generation);
        }

        bool get_entity_id(Scene &_scene, entt::entity _entity, std::string &_out)
        {
            auto &registry = _scene.Registry();
            if (!registry.valid(_entity) || !registry.any_of<uuid::EntityUUID>(_entity))
                return false;

            _out = registry.get<uuid::EntityUUID>(_entity).uuid;
            return !_out.empty();
        }

        bool find_entity(const char *_entityID, Scene *&_scene, entt::entity &_entity)
        {
            if (!_entityID)
                return false;

            for (Scene *scene : s_ActiveScenes)
            {
                if (!scene)
                    continue;

                auto view = scene->Registry().view<uuid::EntityUUID>();
                for (auto [entity, id] : view.each())
                {
                    if (id.uuid == _entityID)
                    {
                        _scene = scene;
                        _entity = entity;
                        return true;
                    }
                }
            }

            return false;
        }

        ////////////
        // LOGGING
        ///////////
        PyObject *python_log(PyObject *, PyObject *_args)
        {
            const char *message = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &message))
                return nullptr;

            ANV_LOG_INFO("[Python] %s", message);
            Py_RETURN_NONE;
        }

        PyObject *python_warn(PyObject *, PyObject *_args)
        {
            const char *message = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &message))
                return nullptr;

            ANV_LOG_WARN("[Python] %s", message);
            Py_RETURN_NONE;
        }

        PyObject *python_error(PyObject *, PyObject *_args)
        {
            const char *message = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &message))
                return nullptr;

            ANV_LOG_ERROR("[Python] %s", message);
            Py_RETURN_NONE;
        }

        ////////////
        /// TRANSFORM
        ////////////
        PyObject *python_get_position(PyObject *, PyObject *_args)
        {
            const char *entityID = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &entityID))
                return nullptr;

            Scene *scene = nullptr;
            entt::entity entity = entt::null;
            if (!find_entity(entityID, scene, entity) ||
                !scene->HasComponent<Component::Transform2d>(entity))
            {
                PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
                return nullptr;
            }

            const auto &transform = scene->GetComponent<Component::Transform2d>(entity);
            return Py_BuildValue("(ff)", transform.position.x, transform.position.y);
        }

        PyObject *python_set_position(PyObject *, PyObject *_args)
        {
            const char *entityID = nullptr;
            float x = 0.0f;
            float y = 0.0f;
            if (!PyArg_ParseTuple(_args, "sff", &entityID, &x, &y))
                return nullptr;

            Scene *scene = nullptr;
            entt::entity entity = entt::null;
            if (!find_entity(entityID, scene, entity) ||
                !scene->HasComponent<Component::Transform2d>(entity))
            {
                PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
                return nullptr;
            }

            scene->GetComponent<Component::Transform2d>(entity).position = {x, y};
            Py_RETURN_NONE;
        }

        PyObject *python_get_rotation(PyObject *, PyObject *_args)
        {
            const char *entityID = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &entityID))
                return nullptr;

            Scene *scene = nullptr;
            entt::entity entity = entt::null;
            if (!find_entity(entityID, scene, entity) ||
                !scene->HasComponent<Component::Transform2d>(entity))
            {
                PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
                return nullptr;
            }

            return PyFloat_FromDouble(scene->GetComponent<Component::Transform2d>(entity).rotation);
        }

        PyObject *python_set_rotation(PyObject *, PyObject *_args)
        {
            const char *entityID = nullptr;
            float rotation = 0.0f;
            if (!PyArg_ParseTuple(_args, "sf", &entityID, &rotation))
                return nullptr;

            Scene *scene = nullptr;
            entt::entity entity = entt::null;
            if (!find_entity(entityID, scene, entity) ||
                !scene->HasComponent<Component::Transform2d>(entity))
            {
                PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
                return nullptr;
            }

            scene->GetComponent<Component::Transform2d>(entity).rotation = rotation;
            Py_RETURN_NONE;
        }

        PyMethodDef s_AnvilMethods[] = {
            {"log", python_log, METH_VARARGS, "Write an informational message to the Anvil log."},
            {"warn", python_warn, METH_VARARGS, "Write a warning to the Anvil log."},
            {"error", python_error, METH_VARARGS, "Write an error to the Anvil log."},
            {"_get_position", python_get_position, METH_VARARGS, nullptr},
            {"_set_position", python_set_position, METH_VARARGS, nullptr},
            {"_get_rotation", python_get_rotation, METH_VARARGS, nullptr},
            {"_set_rotation", python_set_rotation, METH_VARARGS, nullptr},
            {nullptr, nullptr, 0, nullptr}};

        PyModuleDef s_AnvilNativeModule = {
            PyModuleDef_HEAD_INIT,
            "_anvil",
            "Native Anvil engine bindings.",
            -1,
            s_AnvilMethods};

        PyMODINIT_FUNC PyInit__anvil()
        {
            return PyModule_Create(&s_AnvilNativeModule);
        }

        PyObject *load_module(const std::filesystem::path &_path, ModuleRecord &_record)
        {
            PyObject *importlib = PyImport_ImportModule("importlib.util");
            if (!importlib)
                return nullptr;

            const std::string name = module_name(_path, ++_record.generation);
            PyObject *spec = PyObject_CallMethod(
                importlib,
                "spec_from_file_location",
                "ss",
                name.c_str(),
                _path.string().c_str());
            Py_DECREF(importlib);

            if (!spec)
                return nullptr;

            PyObject *module = PyObject_CallMethod(
                PyImport_ImportModule("importlib.util"),
                "module_from_spec",
                "O",
                spec);
            if (!module)
            {
                Py_DECREF(spec);
                return nullptr;
            }

            PyObject *loader = PyObject_GetAttrString(spec, "loader");
            PyObject *result = loader ? PyObject_CallMethod(loader, "exec_module", "O", module) : nullptr;
            Py_XDECREF(loader);
            Py_DECREF(spec);

            if (!result)
            {
                Py_DECREF(module);
                return nullptr;
            }
            Py_DECREF(result);

            Py_XDECREF(_record.module);
            _record.module = module;
            std::error_code error;
            _record.lastWriteTime = std::filesystem::last_write_time(_path, error);
            return module;
        }

        ScriptFieldType annotation_type(PyObject *_annotation)
        {
            if (!_annotation)
                return ScriptFieldType::None;

            if (_annotation == reinterpret_cast<PyObject *>(&PyBool_Type))
                return ScriptFieldType::Bool;
            if (_annotation == reinterpret_cast<PyObject *>(&PyLong_Type))
                return ScriptFieldType::Int;
            if (_annotation == reinterpret_cast<PyObject *>(&PyFloat_Type))
                return ScriptFieldType::Float;
            if (_annotation == reinterpret_cast<PyObject *>(&PyUnicode_Type))
                return ScriptFieldType::String;

            if (PyUnicode_Check(_annotation))
            {
                const char *name = PyUnicode_AsUTF8(_annotation);
                if (!name)
                    return ScriptFieldType::None;
                if (std::string_view(name) == "bool")
                    return ScriptFieldType::Bool;
                if (std::string_view(name) == "int")
                    return ScriptFieldType::Int;
                if (std::string_view(name) == "float")
                    return ScriptFieldType::Float;
                if (std::string_view(name) == "str")
                    return ScriptFieldType::String;
            }

            return ScriptFieldType::None;
        }

        std::string python_value_to_string(PyObject *_value, ScriptFieldType _type)
        {
            if (!_value)
                return {};

            switch (_type)
            {
            case ScriptFieldType::Bool:
                return PyObject_IsTrue(_value) ? "true" : "false";
            case ScriptFieldType::Int:
                return std::to_string(PyLong_AsLongLong(_value));
            case ScriptFieldType::Float:
                return std::to_string(PyFloat_AsDouble(_value));
            case ScriptFieldType::String:
            {
                const char *value = PyUnicode_AsUTF8(_value);
                return value ? value : "";
            }
            default:
                return {};
            }
        }

        PyObject *string_to_python_value(const ScriptField &_field)
        {
            switch (_field.type)
            {
            case ScriptFieldType::Bool:
                return PyBool_FromLong(_field.value == "true" || _field.value == "1");
            case ScriptFieldType::Int:
            {
                long long value = 0;
                std::from_chars(_field.value.data(), _field.value.data() + _field.value.size(), value);
                return PyLong_FromLongLong(value);
            }
            case ScriptFieldType::Float:
            {
                try
                {
                    return PyFloat_FromDouble(std::stod(_field.value));
                }
                catch (...)
                {
                    return PyFloat_FromDouble(0.0);
                }
            }
            case ScriptFieldType::String:
                return PyUnicode_FromString(_field.value.c_str());
            default:
                Py_RETURN_NONE;
            }
        }

        void reflect_fields(PyObject *_classObject, Component::Script &_component)
        {
            PyObject *annotations = PyObject_GetAttrString(_classObject, "__annotations__");
            if (!annotations)
            {
                PyErr_Clear();
                return;
            }

            if (!PyDict_Check(annotations))
            {
                Py_DECREF(annotations);
                return;
            }

            PyObject *key = nullptr;
            PyObject *annotation = nullptr;
            Py_ssize_t position = 0;
            while (PyDict_Next(annotations, &position, &key, &annotation))
            {
                const char *fieldName = PyUnicode_Check(key) ? PyUnicode_AsUTF8(key) : nullptr;
                if (!fieldName)
                    continue;

                const ScriptFieldType type = annotation_type(annotation);
                if (type == ScriptFieldType::None)
                {
                    ANV_LOG_WARN("Unsupported Python field annotation '%s.%s'.", _component.className.c_str(), fieldName);
                    continue;
                }

                if (_component.fields.contains(fieldName))
                    continue;

                PyObject *defaultValue = PyObject_GetAttrString(_classObject, fieldName);
                ScriptField field;
                field.type = type;
                field.value = python_value_to_string(defaultValue, type);
                Py_XDECREF(defaultValue);
                _component.fields.emplace(fieldName, std::move(field));
            }

            Py_DECREF(annotations);
        }

        void save_instance_fields(RuntimeInstance &_instance)
        {
            if (!_instance.scene || !_instance.object)
                return;

            auto &registry = _instance.scene->Registry();
            if (!registry.valid(_instance.entity) || !registry.any_of<Component::Script>(_instance.entity))
                return;

            auto &component = registry.get<Component::Script>(_instance.entity);
            for (auto &[name, field] : component.fields)
            {
                PyObject *value = PyObject_GetAttrString(_instance.object, name.c_str());
                if (!value)
                {
                    PyErr_Clear();
                    continue;
                }

                field.value = python_value_to_string(value, field.type);
                Py_DECREF(value);
            }
        }

        bool call_method(PyObject *_object, const char *_name, float *_deltaTime = nullptr)
        {
            PyObject *method = PyObject_GetAttrString(_object, _name);
            if (!method)
            {
                PyErr_Clear();
                return true;
            }

            if (!PyCallable_Check(method))
            {
                Py_DECREF(method);
                return true;
            }

            PyObject *result = _deltaTime
                                   ? PyObject_CallFunction(method, "f", *_deltaTime)
                                   : PyObject_CallNoArgs(method);
            Py_DECREF(method);

            if (!result)
                return false;

            Py_DECREF(result);
            return true;
        }
#endif
    } // Empty Namespace

    bool PythonScriptEngine::Initialize(const std::filesystem::path &_projectDirectory)
    {
        if (s_Initialized)
            return true;

        s_ProjectDirectory = _projectDirectory;
        s_ScriptsDirectory = s_ProjectDirectory / "Scripts";

        s_PythonModuleDirectory = s_ExecDir / "Anvil" / "Resources" / "Python";

        if (!std::filesystem::exists(s_PythonModuleDirectory))
        {
            ANV_LOG_ERROR(
                "Anvil Python API directory was not found: '%s'",
                s_PythonModuleDirectory.string().c_str());

            return false;
        }

#ifndef ANV_ENABLE_PYTHON
        ANV_LOG_WARN("Python scripting is disabled for this build.");
        return false;
#else
        if (PyImport_AppendInittab("_anvil", &PyInit__anvil) == -1)
        {
            ANV_LOG_ERROR("Failed to register Anvil's native Python module.");
            return false;
        }

        Py_Initialize();
        if (!Py_IsInitialized())
        {
            ANV_LOG_ERROR("Failed to initialize the embedded Python interpreter.");
            return false;
        }

        s_Initialized = true;

        std::error_code error;
        std::filesystem::create_directories(s_ScriptsDirectory, error);

        PyObject *sysPath = PySys_GetObject("path");
        PyObject *scriptsPath = PyUnicode_FromString(s_ScriptsDirectory.string().c_str());
        if (!scriptsPath || PyList_Insert(sysPath, 0, scriptsPath) != 0)
        {
            Py_XDECREF(scriptsPath);
            log_python_exception("adding the Scripts directory to sys.path");
            Shutdown();
            return false;
        }
        Py_DECREF(scriptsPath);

        PyObject *modulePath = PyUnicode_FromString(s_PythonModuleDirectory.string().c_str());
        if (!modulePath || PyList_Insert(sysPath, 0, modulePath) != 0)
        {
            Py_XDECREF(modulePath);
            log_python_exception("adding the anvil API module directory to sys.path");
            Shutdown();
            return false;
        }
        Py_DECREF(modulePath);

        PyObject *anvilModule =
            PyImport_ImportModule("anvil");

        if (!anvilModule)
        {
            log_python_exception("importing the Anvil Python API");
            Shutdown();
            return false;
        }

        Py_DECREF(anvilModule);

        ANV_LOG_INFO(
            "Anvil Python API: '%s'",
            s_PythonModuleDirectory.string().c_str());

        ANV_LOG_INFO(
            "Project Scripts: '%s'",
            s_ScriptsDirectory.string().c_str());
        return true;
#endif
    }

    void PythonScriptEngine::Shutdown()
    {
#ifdef ANV_ENABLE_PYTHON
        if (!s_Initialized)
            return;

        for (auto &[entityID, instance] : s_Instances)
        {
            save_instance_fields(instance);
            if (!call_method(instance.object, "on_destroy"))
                log_python_exception("calling on_destroy during Python shutdown");
            Py_XDECREF(instance.object);
        }
        s_Instances.clear();

        for (auto &[path, module] : s_Modules)
            Py_XDECREF(module.module);
        s_Modules.clear();

        if (Py_FinalizeEx() < 0)
            ANV_LOG_ERROR("Python interpreter shutdown reported an error.");
#endif

        s_ActiveScenes.clear();
        s_ProjectDirectory.clear();
        s_ScriptsDirectory.clear();
        s_Initialized = false;
        s_ReloadAll = false;
        s_RequestedReloadPath.clear();
    }

    bool PythonScriptEngine::IsInitialized()
    {
        return s_Initialized;
    }

    void PythonScriptEngine::UpdateScene(Scene &_scene, float _deltaTime)
    {
#ifndef ANV_ENABLE_PYTHON
        (void)_scene;
        (void)_deltaTime;
#else
        if (!s_Initialized)
            return;

        s_ActiveScenes.insert(&_scene);
        reload_changed_modules(_scene);

        auto view = _scene.Registry().view<uuid::EntityUUID, Component::Script>();
        for (auto [entity, id, script] : view.each())
        {
            if (!script.enabled || script.modulePath.empty() || script.className.empty())
                continue;

            if (!s_Instances.contains(id.uuid) && !create_instance(_scene, entity))
                continue;

            auto instance = s_Instances.find(id.uuid);
            if (instance == s_Instances.end() || !instance->second.object)
                continue;

            if (!call_method(instance->second.object, "on_update", &_deltaTime))
                log_python_exception("calling script on_update");
        }
#endif
    }

    void PythonScriptEngine::ShutdownScene(Scene &_scene)
    {
#ifdef ANV_ENABLE_PYTHON
        for (auto iterator = s_Instances.begin(); iterator != s_Instances.end();)
        {
            if (iterator->second.scene != &_scene)
            {
                ++iterator;
                continue;
            }

            save_instance_fields(iterator->second);
            if (!call_method(iterator->second.object, "on_destroy"))
                log_python_exception("calling scene script on_destroy");
            Py_XDECREF(iterator->second.object);
            iterator = s_Instances.erase(iterator);
        }
#endif
        s_ActiveScenes.erase(&_scene);
    }

    void PythonScriptEngine::DestroyEntity(Scene &_scene, entt::entity _entity)
    {
#ifdef ANV_ENABLE_PYTHON
        std::string entityID;
        if (!get_entity_id(_scene, _entity, entityID))
            return;
        destroy_instance(entityID, true);
#else
        (void)_scene;
        (void)_entity;
#endif
    }

    void PythonScriptEngine::RequestReload(const std::filesystem::path &_modulePath)
    {
        if (_modulePath.empty())
        {
            s_ReloadAll = true;
            s_RequestedReloadPath.clear();
            return;
        }

        s_RequestedReloadPath = _modulePath.is_absolute()
                                    ? _modulePath
                                    : s_ScriptsDirectory / _modulePath;
    }

    bool PythonScriptEngine::create_instance(Scene &_scene, entt::entity _entity)
    {
#ifndef ANV_ENABLE_PYTHON
        (void)_scene;
        (void)_entity;
        return false;
#else
        auto &registry = _scene.Registry();
        if (!registry.valid(_entity) ||
            !registry.all_of<uuid::EntityUUID, Component::Script>(_entity))
            return false;

        auto &id = registry.get<uuid::EntityUUID>(_entity);
        auto &component = registry.get<Component::Script>(_entity);
        const std::filesystem::path path = component.modulePath.ends_with(".py")
                                               ? s_ScriptsDirectory / component.modulePath
                                               : s_ScriptsDirectory / (component.modulePath + ".py");

        if (!std::filesystem::exists(path))
        {
            ANV_LOG_ERROR("Python script module was not found: '%s'.", path.string().c_str());
            return false;
        }

        const std::string normalizedPath = normalize_path(path);
        ModuleRecord &record = s_Modules[normalizedPath];
        if (!record.module && !load_module(path, record))
        {
            log_python_exception("loading a script module");
            return false;
        }

        PyObject *classObject = PyObject_GetAttrString(record.module, component.className.c_str());
        if (!classObject)
        {
            log_python_exception("resolving a script class");
            return false;
        }

        PyObject *publicModule = PyImport_ImportModule("anvil");
        PyObject *baseClass = publicModule ? PyObject_GetAttrString(publicModule, "Script") : nullptr;
        const bool validClass = PyType_Check(classObject) && baseClass && PyObject_IsSubclass(classObject, baseClass) == 1;
        Py_XDECREF(baseClass);
        Py_XDECREF(publicModule);

        if (!validClass)
        {
            ANV_LOG_ERROR("Python class '%s' must derive from anvil.Script.", component.className.c_str());
            Py_DECREF(classObject);
            return false;
        }

        reflect_fields(classObject, component);
        PyObject *object = PyObject_CallNoArgs(classObject);
        Py_DECREF(classObject);
        if (!object)
        {
            log_python_exception("constructing a script instance");
            return false;
        }

        PyObject *entityID = PyUnicode_FromString(id.uuid.c_str());
        if (!entityID || PyObject_SetAttrString(object, "entity_id", entityID) != 0)
        {
            Py_XDECREF(entityID);
            Py_DECREF(object);
            log_python_exception("binding an entity to a script instance");
            return false;
        }
        Py_DECREF(entityID);

        for (const auto &[name, field] : component.fields)
        {
            PyObject *value = string_to_python_value(field);
            if (!value || PyObject_SetAttrString(object, name.c_str(), value) != 0)
            {
                Py_XDECREF(value);
                Py_DECREF(object);
                log_python_exception("restoring a reflected script field");
                return false;
            }
            Py_DECREF(value);
        }

        RuntimeInstance instance;
        instance.scene = &_scene;
        instance.entity = _entity;
        instance.modulePath = path;
        instance.object = object;
        s_Instances[id.uuid] = instance;

        if (!call_method(object, "on_create"))
        {
            log_python_exception("calling script on_create");
            destroy_instance(id.uuid, false);
            return false;
        }

        ANV_LOG_INFO("Created Python script '%s.%s' for entity '%s'.",
                     component.modulePath.c_str(), component.className.c_str(), id.uuid.c_str());
        return true;
#endif
    }

    void PythonScriptEngine::destroy_instance(const std::string &_entityID, bool _invokeDestroy)
    {
#ifdef ANV_ENABLE_PYTHON
        auto iterator = s_Instances.find(_entityID);
        if (iterator == s_Instances.end())
            return;

        save_instance_fields(iterator->second);
        if (_invokeDestroy && !call_method(iterator->second.object, "on_destroy"))
            log_python_exception("calling script on_destroy");

        Py_XDECREF(iterator->second.object);
        s_Instances.erase(iterator);
#else
        (void)_entityID;
        (void)_invokeDestroy;
#endif
    }

    void PythonScriptEngine::reload_changed_modules(Scene &_scene)
    {
#ifndef ANV_ENABLE_PYTHON
        (void)_scene;
#else
        std::unordered_set<std::string> reloadPaths;
        auto view = _scene.Registry().view<Component::Script>();
        for (auto [entity, script] : view.each())
        {
            if (script.modulePath.empty())
                continue;

            const std::filesystem::path path = script.modulePath.ends_with(".py")
                                                   ? s_ScriptsDirectory / script.modulePath
                                                   : s_ScriptsDirectory / (script.modulePath + ".py");
            const std::string normalized = normalize_path(path);

            auto module = s_Modules.find(normalized);
            if (module == s_Modules.end() || !std::filesystem::exists(path))
                continue;

            std::error_code error;
            const auto writeTime = std::filesystem::last_write_time(path, error);
            const bool requested = s_ReloadAll ||
                                   (!s_RequestedReloadPath.empty() && normalize_path(s_RequestedReloadPath) == normalized);
            if (requested || (!error && writeTime != module->second.lastWriteTime))
                reloadPaths.insert(normalized);
        }

        for (const std::string &path : reloadPaths)
        {
            for (auto iterator = s_Instances.begin(); iterator != s_Instances.end();)
            {
                if (normalize_path(iterator->second.modulePath) != path)
                {
                    ++iterator;
                    continue;
                }

                save_instance_fields(iterator->second);
                if (!call_method(iterator->second.object, "on_destroy"))
                    log_python_exception("calling on_destroy before hot reload");
                Py_XDECREF(iterator->second.object);
                iterator = s_Instances.erase(iterator);
            }

            ModuleRecord &record = s_Modules[path];
            if (!load_module(path, record))
            {
                log_python_exception("hot reloading a script module");
                continue;
            }

            ANV_LOG_INFO("Hot reloaded Python module '%s'.", path.c_str());
        }

        s_ReloadAll = false;
        s_RequestedReloadPath.clear();
#endif
    }

    void PythonScriptEngine::log_python_exception(const char *_context)
    {
#ifdef ANV_ENABLE_PYTHON
        if (!PyErr_Occurred())
            return;

        PyObject *tracebackModule = PyImport_ImportModule("traceback");
        PyObject *formatted = tracebackModule
                                  ? PyObject_CallMethod(tracebackModule, "format_exc", nullptr)
                                  : nullptr;
        const char *message = formatted ? PyUnicode_AsUTF8(formatted) : nullptr;

        ANV_LOG_ERROR("Python exception while %s:\n%s",
                      _context,
                      message ? message : "Unknown Python error");

        Py_XDECREF(formatted);
        Py_XDECREF(tracebackModule);
        PyErr_Clear();
#else
        (void)_context;
#endif
    }
}

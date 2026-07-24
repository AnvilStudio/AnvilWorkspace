#include "PythonScriptEngine.h"

#include "Util/UMacros.h"

#ifdef ANV_ENABLE_PYTHON
#include <Python.h>
#endif

#include <string>

namespace anv
{
    namespace
    {
        bool s_Initialized = false;
        std::filesystem::path s_ProjectDirectory;

#ifdef ANV_ENABLE_PYTHON
        PyObject* s_MainModule = nullptr;

        PyObject* python_log(PyObject*, PyObject* _args)
        {
            const char* message = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &message))
                return nullptr;

            ANV_LOG_INFO("[Python] %s", message);
            Py_RETURN_NONE;
        }

        PyObject* python_warn(PyObject*, PyObject* _args)
        {
            const char* message = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &message))
                return nullptr;

            ANV_LOG_WARN("[Python] %s", message);
            Py_RETURN_NONE;
        }

        PyObject* python_error(PyObject*, PyObject* _args)
        {
            const char* message = nullptr;
            if (!PyArg_ParseTuple(_args, "s", &message))
                return nullptr;

            ANV_LOG_ERROR("[Python] %s", message);
            Py_RETURN_NONE;
        }

        PyMethodDef s_AnvilMethods[] = {
            {"log", python_log, METH_VARARGS, "Writes an informational message to the Anvil log."},
            {"warn", python_warn, METH_VARARGS, "Writes a warning to the Anvil log."},
            {"error", python_error, METH_VARARGS, "Writes an error to the Anvil log."},
            {nullptr, nullptr, 0, nullptr}
        };

        PyModuleDef s_AnvilModule = {
            PyModuleDef_HEAD_INIT,
            "_anvil",
            "Native Anvil engine bindings.",
            -1,
            s_AnvilMethods
        };

        PyMODINIT_FUNC PyInit__anvil()
        {
            return PyModule_Create(&s_AnvilModule);
        }
#endif
    }

    bool PythonScriptEngine::Initialize(const std::filesystem::path& _projectDirectory)
    {
        if (s_Initialized)
            return true;

        s_ProjectDirectory = _projectDirectory;

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

        PyObject* nativeModule = PyImport_ImportModule("_anvil");
        if (!nativeModule)
        {
            log_python_exception("importing the native _anvil module");
            Shutdown();
            return false;
        }

        PyObject* modules = PyImport_GetModuleDict();
        if (PyDict_SetItemString(modules, "anvil", nativeModule) != 0)
        {
            Py_DECREF(nativeModule);
            log_python_exception("registering the public anvil module");
            Shutdown();
            return false;
        }
        Py_DECREF(nativeModule);

        if (!load_main_module())
        {
            Shutdown();
            return false;
        }

        ANV_LOG_INFO("Python scripting initialized.");
        call_no_argument_callback("on_create");
        return true;
#endif
    }

    void PythonScriptEngine::Update(float _deltaTime)
    {
#ifdef ANV_ENABLE_PYTHON
        if (!s_Initialized || !s_MainModule)
            return;

        PyObject* callback = PyObject_GetAttrString(s_MainModule, "on_update");
        if (!callback)
        {
            PyErr_Clear();
            return;
        }

        if (!PyCallable_Check(callback))
        {
            Py_DECREF(callback);
            return;
        }

        PyObject* result = PyObject_CallFunction(callback, "f", _deltaTime);
        Py_DECREF(callback);

        if (!result)
        {
            log_python_exception("calling on_update");
            return;
        }

        Py_DECREF(result);
#else
        (void)_deltaTime;
#endif
    }

    void PythonScriptEngine::Shutdown()
    {
#ifdef ANV_ENABLE_PYTHON
        if (!s_Initialized)
            return;

        call_no_argument_callback("on_destroy");
        Py_CLEAR(s_MainModule);

        const int finalizeResult = Py_FinalizeEx();
        if (finalizeResult < 0)
            ANV_LOG_ERROR("Python interpreter shutdown reported an error.");
#endif

        s_Initialized = false;
        s_ProjectDirectory.clear();
    }

    bool PythonScriptEngine::IsInitialized()
    {
        return s_Initialized;
    }

    bool PythonScriptEngine::load_main_module()
    {
#ifndef ANV_ENABLE_PYTHON
        return false;
#else
        const std::filesystem::path scriptsDirectory = s_ProjectDirectory / "Scripts";
        const std::filesystem::path mainScript = scriptsDirectory / "main.py";

        if (!std::filesystem::exists(mainScript))
        {
            ANV_LOG_INFO("No Python entry script found at '%s'.", mainScript.string().c_str());
            return true;
        }

        PyObject* sysPath = PySys_GetObject("path");
        PyObject* scriptsPath = PyUnicode_FromString(scriptsDirectory.string().c_str());
        if (!scriptsPath || PyList_Insert(sysPath, 0, scriptsPath) != 0)
        {
            Py_XDECREF(scriptsPath);
            log_python_exception("adding the project Scripts directory to sys.path");
            return false;
        }
        Py_DECREF(scriptsPath);

        s_MainModule = PyImport_ImportModule("main");
        if (!s_MainModule)
        {
            log_python_exception("importing Scripts/main.py");
            return false;
        }

        return true;
#endif
    }

    bool PythonScriptEngine::call_no_argument_callback(const char* _callbackName)
    {
#ifndef ANV_ENABLE_PYTHON
        (void)_callbackName;
        return false;
#else
        if (!s_MainModule)
            return true;

        PyObject* callback = PyObject_GetAttrString(s_MainModule, _callbackName);
        if (!callback)
        {
            PyErr_Clear();
            return true;
        }

        if (!PyCallable_Check(callback))
        {
            Py_DECREF(callback);
            return true;
        }

        PyObject* result = PyObject_CallNoArgs(callback);
        Py_DECREF(callback);

        if (!result)
        {
            log_python_exception(_callbackName);
            return false;
        }

        Py_DECREF(result);
        return true;
#endif
    }

    void PythonScriptEngine::log_python_exception(const char* _context)
    {
#ifdef ANV_ENABLE_PYTHON
        if (!PyErr_Occurred())
            return;

        PyObject* exceptionType = nullptr;
        PyObject* exceptionValue = nullptr;
        PyObject* exceptionTraceback = nullptr;
        PyErr_Fetch(&exceptionType, &exceptionValue, &exceptionTraceback);
        PyErr_NormalizeException(&exceptionType, &exceptionValue, &exceptionTraceback);

        PyObject* messageObject = exceptionValue ? PyObject_Str(exceptionValue) : nullptr;
        const char* message = messageObject ? PyUnicode_AsUTF8(messageObject) : "Unknown Python error";

        ANV_LOG_ERROR("Python exception while %s: %s", _context, message ? message : "Unknown Python error");

        Py_XDECREF(messageObject);
        Py_XDECREF(exceptionType);
        Py_XDECREF(exceptionValue);
        Py_XDECREF(exceptionTraceback);
#endif
    }
}

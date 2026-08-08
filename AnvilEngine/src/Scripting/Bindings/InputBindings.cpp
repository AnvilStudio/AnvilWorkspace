#include "PythonBindings.h"

#ifdef ANV_ENABLE_PYTHON
#include "../../Core/App.h"

namespace anv::python
{
    namespace
    {
        _shared<InputSystem> ResolveInputSystem()
        {
            App* app = App::GetInstance();
            if (!app)
            {
                PyErr_SetString(
                    PyExc_RuntimeError,
                    "The Anvil application instance is unavailable.");
                return nullptr;
            }

            auto inputSystem = app->GetInputSystem();
            if (!inputSystem)
            {
                PyErr_SetString(
                    PyExc_RuntimeError,
                    "The Anvil input system is unavailable.");
                return nullptr;
            }

            return inputSystem;
        }
    }

    PyObject* IsKeyPressed(PyObject*, PyObject* args)
    {
        int key = 0;
        if (!PyArg_ParseTuple(args, "i", &key))
            return nullptr;

        auto inputSystem = ResolveInputSystem();
        if (!inputSystem)
            return nullptr;

        return PyBool_FromLong(inputSystem->IsKeyPressed(key) ? 1 : 0);
    }

    PyObject* IsKeyJustPressed(PyObject*, PyObject* args)
    {
        int key = 0;
        if (!PyArg_ParseTuple(args, "i", &key))
            return nullptr;

        auto inputSystem = ResolveInputSystem();
        if (!inputSystem)
            return nullptr;

        return PyBool_FromLong(inputSystem->IsKeyJustPressed(key) ? 1 : 0);
    }
}
#endif

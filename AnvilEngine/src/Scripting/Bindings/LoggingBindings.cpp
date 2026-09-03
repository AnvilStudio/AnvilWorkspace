#include "PythonBindings.h"

#ifdef ANV_ENABLE_PYTHON
#include "../../Util/UMacros.h"

namespace anv::python
{
    PyObject* Log(PyObject*, PyObject* args)
    {
        const char* message = nullptr;
        if (!PyArg_ParseTuple(args, "s", &message))
            return nullptr;

        ANV_LOG_INFO("[Python] %s", message);
        Py_RETURN_NONE;
    }

    PyObject* Warn(PyObject*, PyObject* args)
    {
        const char* message = nullptr;
        if (!PyArg_ParseTuple(args, "s", &message))
            return nullptr;

        ANV_LOG_WARN("[Python] %s", message);
        Py_RETURN_NONE;
    }

    PyObject* Error(PyObject*, PyObject* args)
    {
        const char* message = nullptr;
        if (!PyArg_ParseTuple(args, "s", &message))
            return nullptr;

        ANV_LOG_ERROR("[Python] %s", message);
        Py_RETURN_NONE;
    }
}
#endif

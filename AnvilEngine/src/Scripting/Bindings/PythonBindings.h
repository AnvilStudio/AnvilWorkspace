#pragma once

#ifdef ANV_ENABLE_PYTHON
#include <Python.h>

namespace anv::python
{
    PyObject* Log(PyObject* self, PyObject* args);
    PyObject* Warn(PyObject* self, PyObject* args);
    PyObject* Error(PyObject* self, PyObject* args);

    PyObject* IsKeyPressed(PyObject* self, PyObject* args);
}
#endif

#pragma once

#ifdef ANV_ENABLE_PYTHON
#include <Python.h>

namespace anv::python
{
    PyObject* Log(PyObject* self, PyObject* args);
    PyObject* Warn(PyObject* self, PyObject* args);
    PyObject* Error(PyObject* self, PyObject* args);

    PyObject* IsKeyPressed(PyObject* self, PyObject* args);
    PyObject* IsKeyJustPressed(PyObject* self, PyObject* args);

    PyObject* HasComponent(PyObject* self, PyObject* args);

    PyObject* GetPosition(PyObject* self, PyObject* args);
    PyObject* SetPosition(PyObject* self, PyObject* args);
    PyObject* GetRotation(PyObject* self, PyObject* args);
    PyObject* SetRotation(PyObject* self, PyObject* args);

    PyObject* RigidBodyAddForce(PyObject* self, PyObject* args);
    PyObject* RigidBodyApplyImpulse(PyObject* self, PyObject* args);
    PyObject* RigidBodyGetLinearVelocity(PyObject* self, PyObject* args);
    PyObject* RigidBodySetLinearVelocity(PyObject* self, PyObject* args);
}
#endif

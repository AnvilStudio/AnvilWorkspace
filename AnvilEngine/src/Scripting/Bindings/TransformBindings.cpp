#include "PythonBindings.h"

#include "../ScriptEntityContext.h"
#include "../../Scene/Component.h"
#include "../../Scene/Scene.h"

namespace anv::python
{
    PyObject* GetPosition(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Transform2d>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
            return nullptr;
        }

        const auto& transform = context.scene->GetComponent<Component::Transform2d>(context.entity);
        return Py_BuildValue("(ff)", transform.position.x, transform.position.y);
    }

    PyObject* SetPosition(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        float x = 0.0f;
        float y = 0.0f;

        if (!PyArg_ParseTuple(args, "sff", &entityID, &x, &y))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Transform2d>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
            return nullptr;
        }

        context.scene->GetComponent<Component::Transform2d>(context.entity).position = {x, y};
        Py_RETURN_NONE;
    }

    PyObject* GetRotation(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Transform2d>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
            return nullptr;
        }

        return PyFloat_FromDouble(
            context.scene->GetComponent<Component::Transform2d>(context.entity).rotation);
    }

    PyObject* SetRotation(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        float rotation = 0.0f;

        if (!PyArg_ParseTuple(args, "sf", &entityID, &rotation))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Transform2d>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity or Transform2d component was not found");
            return nullptr;
        }

        context.scene->GetComponent<Component::Transform2d>(context.entity).rotation = rotation;
        Py_RETURN_NONE;
    }
}

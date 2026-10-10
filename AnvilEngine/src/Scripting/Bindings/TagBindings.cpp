#include "PythonBindings.h"

#ifdef ANV_ENABLE_PYTHON
#include "../ScriptEntityContext.h"
#include <Scene/Component.h>
#include <Scene/Scene.h>

namespace anv::python
{
    PyObject* GetTag(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Tag>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity or Tag component was not found");
            return nullptr;
        }

        const auto& tag = context.scene->GetComponent<Component::Tag>(context.entity);
        return Py_BuildValue("s", tag.value.c_str());
    }

    PyObject* SetTag(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        const char* value;

        if (!PyArg_ParseTuple(args, "ss", &entityID, &value))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Tag>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity or Tag component was not found");
            return nullptr;
        }

        context.scene->GetComponent<Component::Tag>(context.entity).value = std::string(value);
        Py_RETURN_NONE;
    }
}

#endif
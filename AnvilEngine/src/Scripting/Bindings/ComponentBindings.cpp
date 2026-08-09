#include "PythonBindings.h"

#ifdef ANV_ENABLE_PYTHON
#include "../ScriptEntityContext.h"
#include "../../Scene/Component.h"
#include "../../Scene/Scene.h"

#include <string_view>

namespace anv::python
{
    PyObject* HasComponent(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        const char* componentName = nullptr;

        if (!PyArg_ParseTuple(args, "ss", &entityID, &componentName))
            return nullptr;

        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context)
            Py_RETURN_FALSE;

        const std::string_view name = componentName ? componentName : "";
        bool hasComponent = false;

        if (name == "Transform2D")
            hasComponent = context.scene->HasComponent<Component::Transform2d>(context.entity);
        else if (name == "RigidBody2D")
            hasComponent = context.scene->HasComponent<Component::Rigidbody2D>(context.entity);
        else if (name == "BoxCollider2D")
            hasComponent = context.scene->HasComponent<Component::BoxCollider2D>(context.entity);
        else
        {
            PyErr_Format(PyExc_ValueError, "Unknown Anvil component type '%s'", componentName);
            return nullptr;
        }

        return PyBool_FromLong(hasComponent ? 1 : 0);
    }

    PyObject* FindEntityByName(PyObject*, PyObject* args)
    {
        const char* entityName = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityName))
            return nullptr;

        const auto context = ResolveScriptEntityByName(entityName ? entityName : "");
        if (!context || !context.scene->HasComponent<uuid::EntityUUID>(context.entity))
            Py_RETURN_NONE;

        const auto& id = context.scene->GetComponent<uuid::EntityUUID>(context.entity);
        return PyUnicode_FromString(id.uuid.c_str());
    }
}
#endif

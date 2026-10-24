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
            hasComponent = context.scene->HasComponent<Component::Transform2D>(context.entity);
        else if (name == "RigidBody2D")
            hasComponent = context.scene->HasComponent<Component::Rigidbody2D>(context.entity);
        else if (name == "BoxCollider2D")
            hasComponent = context.scene->HasComponent<Component::BoxCollider2D>(context.entity);
        else if (name == "Animator2D")
            hasComponent = context.scene->HasComponent<Component::Animator2D>(context.entity);
        else if (name == "TagComponent")
            hasComponent = context.scene->HasComponent<Component::Tag>(context.entity);
        else
        {
            PyErr_Format(PyExc_ValueError, "Unknown Anvil component type '%s'", componentName);
            return nullptr;
        }

        return PyBool_FromLong(hasComponent ? 1 : 0);
    }

    static Component::Animator2D* ResolveAnimator(const char* entityID)
    {
        const auto context = ResolveScriptEntity(entityID ? entityID : "");
        if (!context || !context.scene->HasComponent<Component::Animator2D>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity has no Animator2D component");
            return nullptr;
        }
        return &context.scene->GetComponent<Component::Animator2D>(context.entity);
    }

    PyObject* AnimatorPlayState(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        const char* state = nullptr;
        if (!PyArg_ParseTuple(args, "ss", &entityID, &state)) return nullptr;
        auto* animator = ResolveAnimator(entityID);
        if (!animator) return nullptr;
        if (!animator->PlayState(state))
        {
            PyErr_Format(PyExc_ValueError, "Unknown or unassigned Animator2D state '%s'", state);
            return nullptr;
        }
        Py_RETURN_NONE;
    }

    PyObject* AnimatorPause(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID)) return nullptr;
        auto* animator = ResolveAnimator(entityID);
        if (!animator) return nullptr;
        animator->Pause();
        Py_RETURN_NONE;
    }

    PyObject* AnimatorStop(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID)) return nullptr;
        auto* animator = ResolveAnimator(entityID);
        if (!animator) return nullptr;
        animator->Stop();
        Py_RETURN_NONE;
    }

    PyObject* AnimatorCurrentState(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID)) return nullptr;
        auto* animator = ResolveAnimator(entityID);
        if (!animator) return nullptr;
        return PyUnicode_FromString(animator->currentState.c_str());
    }

    PyObject* SpriteGetFlip(PyObject*, PyObject* args)
    {
        const char* id = nullptr;
        const char* axis = nullptr;
        if (!PyArg_ParseTuple(args, "ss", &id, &axis)) return nullptr;
        const auto context = ResolveScriptEntity(id ? id : "");
        if (!context || !context.scene->HasComponent<Component::SpriteRenderer>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity has no SpriteRenderer component");
            return nullptr;
        }
        const auto& sprite = context.scene->GetComponent<Component::SpriteRenderer>(context.entity);
        const std::string_view key(axis);
        if (key == "x") return PyBool_FromLong(sprite.flipX);
        if (key == "y") return PyBool_FromLong(sprite.flipY);
        PyErr_SetString(PyExc_ValueError, "Sprite flip axis must be x or y");
        return nullptr;
    }

    PyObject* SpriteSetFlip(PyObject*, PyObject* args)
    {
        const char* id = nullptr;
        const char* axis = nullptr;
        int value = 0;
        if (!PyArg_ParseTuple(args, "ssp", &id, &axis, &value)) return nullptr;
        const auto context = ResolveScriptEntity(id ? id : "");
        if (!context || !context.scene->HasComponent<Component::SpriteRenderer>(context.entity))
        {
            PyErr_SetString(PyExc_KeyError, "Entity has no SpriteRenderer component");
            return nullptr;
        }
        auto& sprite = context.scene->GetComponent<Component::SpriteRenderer>(context.entity);
        const std::string_view key(axis);
        if (key == "x") sprite.flipX = value != 0;
        else if (key == "y") sprite.flipY = value != 0;
        else
        {
            PyErr_SetString(PyExc_ValueError, "Sprite flip axis must be x or y");
            return nullptr;
        }
        Py_RETURN_NONE;
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

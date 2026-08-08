#include "PythonBindings.h"

#ifdef ANV_ENABLE_PYTHON
#include "../ScriptEntityContext.h"
#include "../../Physics/Physics2D.h"
#include "../../Scene/Component.h"
#include "../../Scene/Scene.h"

namespace anv::python
{
    namespace
    {
        bool ResolveRigidBody(const char* entityID, ScriptEntityContext& context, Physics2D*& physics)
        {
            context = ResolveScriptEntity(entityID ? entityID : "");
            if (!context)
            {
                PyErr_SetString(PyExc_KeyError, "Entity was not found");
                return false;
            }

            if (!context.scene->HasComponent<Component::Rigidbody2D>(context.entity))
            {
                PyErr_SetString(PyExc_KeyError, "Entity does not have a Rigidbody2D component");
                return false;
            }

            physics = context.scene->GetPhysics2D();
            if (!physics || !physics->IsRunning())
            {
                PyErr_SetString(PyExc_RuntimeError, "Physics2D is not running for this scene");
                return false;
            }

            return true;
        }
    }

    PyObject* RigidBodyAddForce(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        float x = 0.0f;
        float y = 0.0f;

        if (!PyArg_ParseTuple(args, "sff", &entityID, &x, &y))
            return nullptr;

        ScriptEntityContext context;
        Physics2D* physics = nullptr;
        if (!ResolveRigidBody(entityID, context, physics))
            return nullptr;

        if (!physics->AddForce(context.entity, x, y))
        {
            PyErr_SetString(PyExc_RuntimeError, "Rigidbody2D runtime body is unavailable");
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    PyObject* RigidBodyApplyImpulse(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        float x = 0.0f;
        float y = 0.0f;

        if (!PyArg_ParseTuple(args, "sff", &entityID, &x, &y))
            return nullptr;

        ScriptEntityContext context;
        Physics2D* physics = nullptr;
        if (!ResolveRigidBody(entityID, context, physics))
            return nullptr;

        if (!physics->ApplyImpulse(context.entity, x, y))
        {
            PyErr_SetString(PyExc_RuntimeError, "Rigidbody2D runtime body is unavailable");
            return nullptr;
        }

        Py_RETURN_NONE;
    }
}
#endif

#include "PythonBindings.h"

#ifdef ANV_ENABLE_PYTHON
#include "../ScriptEntityContext.h"
#include "../../Physics/PhysicsBody2D.h"
#include "../../Physics/PhysicsSystem2D.h"
#include "../../Scene/Component.h"
#include "../../Scene/Scene.h"

namespace anv::python
{
    namespace
    {
        bool ResolveRigidBody(
            const char* entityID,
            ScriptEntityContext& context,
            PhysicsBody2D*& body)
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

            PhysicsSystem2D* physics = context.scene->GetPhysicsSystem2D();
            if (!physics || !physics->IsRunning())
            {
                PyErr_SetString(PyExc_RuntimeError, "PhysicsSystem2D is not running for this scene");
                return false;
            }

            body = physics->GetBody(context.entity);
            if (!body || !body->IsValid())
            {
                PyErr_SetString(PyExc_RuntimeError, "Rigidbody2D runtime body is unavailable");
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
        PhysicsBody2D* body = nullptr;
        if (!ResolveRigidBody(entityID, context, body))
            return nullptr;

        if (!body->AddForce(x, y))
        {
            PyErr_SetString(PyExc_RuntimeError, "Failed to apply force to Rigidbody2D");
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
        PhysicsBody2D* body = nullptr;
        if (!ResolveRigidBody(entityID, context, body))
            return nullptr;

        if (!body->ApplyImpulse(x, y))
        {
            PyErr_SetString(PyExc_RuntimeError, "Failed to apply impulse to Rigidbody2D");
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    PyObject* RigidBodyGetLinearVelocity(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        if (!PyArg_ParseTuple(args, "s", &entityID))
            return nullptr;

        ScriptEntityContext context;
        PhysicsBody2D* body = nullptr;
        if (!ResolveRigidBody(entityID, context, body))
            return nullptr;

        const PhysicsVector2D velocity = body->GetLinearVelocity();
        return Py_BuildValue("(ff)", velocity.x, velocity.y);
    }

    PyObject* RigidBodySetLinearVelocity(PyObject*, PyObject* args)
    {
        const char* entityID = nullptr;
        float x = 0.0f;
        float y = 0.0f;

        if (!PyArg_ParseTuple(args, "sff", &entityID, &x, &y))
            return nullptr;

        ScriptEntityContext context;
        PhysicsBody2D* body = nullptr;
        if (!ResolveRigidBody(entityID, context, body))
            return nullptr;

        body->SetLinearVelocity(x, y);
        Py_RETURN_NONE;
    }
}
#endif

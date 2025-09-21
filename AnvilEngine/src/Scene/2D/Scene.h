#pragma once

#include "Render/Camera.h"

namespace anv
{
    //class Scene3D;
    
    class Scene2D
    {
    public:
        Scene2D() = default;
        ~Scene2D() = default;

        void Init();
        void Shutdown();

        void OnUpdate(float deltaTime);
        void Render();

    private:
        Camera2D m_Camera;
    };
}
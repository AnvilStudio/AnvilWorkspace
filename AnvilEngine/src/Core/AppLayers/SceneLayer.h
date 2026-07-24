#pragma once
#include <Scene/Manager.h>
#include <Scene/Scene.h>
#include <Util/UMacros.h>
#include <Layer/Layer.h>

namespace anv
{
    class SceneLayer : public Layer
    {
    public:
        SceneLayer(_shared<SceneManager> _manager);

        void OnAttach() override;
        void OnDetach() override;
        void OnRender() override;
        void OnUpdate(float _dt) override;

    private:
        void refresh_active_scene();

    private:
        _shared<SceneManager> m_ScnMgr;
        Ref<Scene> m_Active;
    };
}

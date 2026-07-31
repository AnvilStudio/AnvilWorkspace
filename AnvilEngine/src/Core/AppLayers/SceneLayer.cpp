#include "SceneLayer.h"

#include <Render/Renderer.h>

namespace anv
{
    SceneLayer::SceneLayer(_shared<SceneManager> _manager)
        : Layer("Scene Layer"),
          m_ScnMgr(std::move(_manager))
    {
    }

    void SceneLayer::OnAttach()
    {
        ANV_ASSERT(m_ScnMgr, "Scene manager null!")
        refresh_active_scene();
        ANV_ASSERT(m_Active, "Active scene null!")
    }

    void SceneLayer::OnDetach()
    {
        if (m_Active)
            m_Active->Shutdown();
    }

    void SceneLayer::OnRender()
    {
        refresh_active_scene();

        if (!m_Active)
            return;

        if (auto camera = m_Active->GetActiveCamera())
            Renderer2D::SetCamera(camera);

        m_Active->Render();
    }

    void SceneLayer::OnUpdate(float _dt)
    {
        refresh_active_scene();

        if (m_Active)
            m_Active->OnUpdate(_dt);
    }

    void SceneLayer::refresh_active_scene()
    {
        Ref<Scene> active = m_ScnMgr ? m_ScnMgr->GetActive() : nullptr;
        if (!active || active == m_Active)
            return;

        m_Active = active;
    }
}

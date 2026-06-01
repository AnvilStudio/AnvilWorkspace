#include "SceneLayer.h"
#include <Render/Renderer.h>
namespace anv
{
	SceneLayer::SceneLayer(_shared<SceneManager> _manager)
		: Layer("Scene Layer"), m_ScnMgr(_manager)
	{
	}

	void SceneLayer::OnAttach()
	{
		ANV_ASSERT(m_ScnMgr, "Scene manager null!");

		m_Active = m_ScnMgr->GetActive();

		ANV_ASSERT(m_Active, "Active scene null!");

		auto camera = m_Active->GetMainCamera();

		ANV_ASSERT(camera, "Main camera null!");

		Renderer2D::SetCamera(camera);
	}

	void SceneLayer::OnDetach()
	{
		m_Active->Shutdown();
	}

	void SceneLayer::OnRender()
	{
		m_Active->Render();
	}

	void SceneLayer::OnUpdate(float dt)
	{
		m_Active->OnUpdate(dt);
	}
}
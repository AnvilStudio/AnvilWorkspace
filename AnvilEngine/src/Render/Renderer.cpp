#include "Renderer.h"
#include "Shader.h"
#include "GraphicsPipeline.h"
#include <Core/App.h>

namespace anv
{

	void Renderer2D::Init(Render2DCreateInfo _info)
	{
		ANV_PROFILE_SCOPE();
		m_RenderCreateInfo = _info;

		m_RenderAPI = _info.pTarget->GetContext()->InitAPI(_info);

	}

	void Renderer2D::Shutdown()
	{
		m_RenderAPI->OnShutdown();
	}

	void Renderer2D::SetCamera(_shared<Camera2D> _main)
	{
		m_MainCamera = _main;
	}


	void Renderer2D::DrawFrame()
	{
		m_RenderAPI->DrawFrame();
	}

}
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
		m_RenderAPI->SetMainCamera(_main);
	}

	void Renderer2D::BeginScene(_shared<Camera2D> camera)
	{
		m_RenderAPI->BeginScene();
	}

	void Renderer2D::EndScene()
	{
		m_RenderAPI->EndScene();
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, Color color)
	{
		m_RenderAPI->DrawQuad(position, size, color);
	}

	void Renderer2D::DrawFrame()
	{
		m_RenderAPI->DrawFrame();
	}

}
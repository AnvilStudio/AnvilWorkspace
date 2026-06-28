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

	void Renderer2D::BeginScene(/*Ref<RenderTarget> _renderTarget*/)
	{
		m_RenderAPI->BeginScene(/*_renderTarget*/);
	}

	void Renderer2D::DrawScene(Ref<RenderTarget> _renderTarget)
	{
		m_RenderAPI->DrawScene(_renderTarget);
	}

	void Renderer2D::EndScene()
	{
		m_RenderAPI->EndScene();
	}

	void anv::Renderer2D::DrawQuad(const glm::vec2& position, float rotation, const glm::vec2& size, glm::vec4 color, int layer)
	{
		m_RenderAPI->DrawQuad(position, rotation, size, color, layer);
	}

	void Renderer2D::DrawFrame()
	{
		m_RenderAPI->DrawFrame();
	}

	void Renderer2D::WaitIdle()
	{
		App::GetInstance()->GetMainWindow()->GetContext()->WaitIdle();
	}

}
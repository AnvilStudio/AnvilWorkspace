#include "Renderer.h"
#include "Shader.h"
#include <Core/App.h>

namespace anv
{
	Renderer2D::Renderer2D(Render2DCreateInfo _info)
		: m_RenderInfo(_info)
	{
		ANV_PROFILE_SCOPE();
		RenderAPICreateInfo info;
		m_RenderAPI = _info.pTarget->GetContext()->InitAPI(info);

		auto v = Shader::Create(_info.shaderPath + "/shader.glsl", _info.pTarget->GetContext());
	}

	Renderer2D::~Renderer2D()
	{
	}

	// start recording commands
	// optional
	// void Renderer2D::BeginScene(Cam& _cam, Scene& scene)
	void Renderer2D::BeginFrame()
	{
		// std::array<Queue> m_RenderQueues[3] = {}
		// ensure render thread is ready to swap queues

	}

	void Renderer2D::EndFrame()
	{
		// Submit rendering cmds to the render thread
		// Render thread will perform the work 
		// Renderer::Submit()
	}
}
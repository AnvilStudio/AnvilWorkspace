#include "Renderer.h"
#include "Shader.h"
#include "GraphicsPipeline.h"
#include <Core/App.h>

namespace anv
{
	Renderer2D::Renderer2D(Render2DCreateInfo _info)
		: m_RenderInfo(_info)
	{
		ANV_PROFILE_SCOPE();
		RenderAPICreateInfo info;
		m_RenderAPI = _info.pTarget->GetContext()->InitAPI(info);

		// == TMP ==
		auto v = Shader::Create(_info.shaderPath + "/shader.glsl", _info.pTarget->GetContext());
		auto p = GraphicsPipeline::Create(_info.pTarget->GetContext());
		p->SetShaderStages(v);
		p->SetVertexInputLayout({});
		p->SetRasterizationSettings({});
		p->SetColorBlendSettings({});
		p->Build();
		// =========
	}

	Renderer2D::~Renderer2D()
	{
	}

	// start recording commands & begin render pass
	// optional
	// Begin scene should batch all like object together. then draw those objects together
	// All objects with the same color, material, geometry, etc should be rendered at once
	// Camera should belong to the scene
	// void Renderer2D::BeginScene(Scene& scene)
	void Renderer2D::BeginFrame()
	{
		// std::array<Queue> m_RenderQueues[3] = {}
		// ensure render thread is ready to swap queues

	}

	void Renderer2D::EndFrame()
	{
		// end render pass and submit rendering cmds to the render thread
		// Render thread will perform the work 
		// Renderer::Submit()
	}
}
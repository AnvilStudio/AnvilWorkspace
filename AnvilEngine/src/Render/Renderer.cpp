#include "Renderer.h"
#include "Shader.h"
#include "GraphicsPipeline.h"
#include <Core/App.h>

namespace anv
{

	void Renderer2D::Init(Render2DCreateInfo _info)
	{
		ANV_PROFILE_SCOPE();
		RenderAPICreateInfo info;
		m_RenderAPI = _info.pTarget->GetContext()->InitAPI(info);

		RenderPassCreateInfo rpinfo{};
		RenderPassCreateInfo::Attachment col_att{};
		col_att.type        = RenderPassCreateInfo::Attachment::AttType::ATT_TY_COLOR;      // drawing colors
		col_att.loadOp      = RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_UNDEF;    // we dont care about the data before
		col_att.storeOp     = RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_STORE; // save the image
		col_att.beginLayout = RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_UNDEF;   // dont care about the layout bc its cleared anyway
		col_att.endLayout   = RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_COLOR_ATT; // going to a colored image

		rpinfo.attachments.push_back(col_att);

		RenderPassCreateInfo::SubpassInfo rpspinfo{
			.colorAttachments = {0},     // ref the first color attach
			.depthStencilAttachment = -1 // no depth att
		};

		rpinfo.subpasses.push_back(rpspinfo);

		auto renderpass = RenderPass::Create(rpinfo, _info.pTarget->GetContext());
		renderpass->Build();

		// == TMP ==
		auto v = Shader::Create(_info.shaderPath + "/shader.glsl", _info.pTarget->GetContext());
		m_Pipeline = GraphicsPipeline::Create(_info.pTarget->GetContext());
		m_Pipeline->SetShaderStages(v);
		m_Pipeline->SetVertexInputLayout({});
		m_Pipeline->SetRasterizationSettings({});
		m_Pipeline->SetColorBlendSettings({});
		m_Pipeline->SetRenderPass(renderpass.get());
		m_Pipeline->Build();
		// =========

		// Start the render thread
		m_RenderCmdChain.Start();
	}

	void Renderer2D::Shutdown()
	{
		// Stop the render thread
		m_RenderCmdChain.Stop();

		m_Pipeline->Destroy();
	}

	// start recording commands & begin render pass
	// optional
	// Begin scene should batch all like object together. then draw those objects together
	// All objects with the same color, material, geometry, etc should be rendered at once
	// Camera should belong to the scene
	// void Renderer2D::BeginScene(Scene& scene)
	void Renderer2D::BeginFrame()
	{

	}

	void Renderer2D::EndFrame()
	{
		// end render pass 

		// Notify the render thread that the main thread is done
		m_RenderCmdChain.NotifyMainDone();

		// Wait for the render thread to finish processing the front queue
		m_RenderCmdChain.WaitForProcessComplete();

		// back -> middle, middle -> front, front -> back
		m_RenderCmdChain.Swap();
	}
}
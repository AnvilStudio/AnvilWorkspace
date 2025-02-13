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
		RenderAPICreateInfo info;
		m_RenderAPI = _info.pTarget->GetContext()->InitAPI(info);

		RenderPassCreateInfo rpinfo{};
		rpinfo.d_name = "GeometryPass";
		
		RenderPass::Attachment colatt;
		colatt.type = RenderPass::Attachment::Type::ATT_TY_COLOR;
		colatt.loadOp = RenderPass::Attachment::LoadOp::LOAD_OP_UNDEF;
		colatt.storeOp = RenderPass::Attachment::StoreOp::STORE_OP_STORE;
		colatt.beginLayout = RenderPass::Attachment::ImgLayout::IMG_LAYOUT_UNDEF;
		colatt.endLayout = RenderPass::Attachment::ImgLayout::IMG_LAYOUT_COLOR_ATT;
		rpinfo.attachments.push_back(colatt);

		RenderPassCreateInfo::SubpassInfo rpspinfo{
			.colorAttachments = {0},     // ref the first color attach
			.depthStencilAttachment = -1 // no depth att
		};

		rpinfo.subpasses.push_back(rpspinfo);

		m_RenderPass = RenderPass::Create(rpinfo, _info.pTarget->GetContext());
		m_RenderPass->Build();

		// == TMP ==
		auto v = Shader::Create(_info.shaderPath + "/shader.glsl", _info.pTarget->GetContext());
		m_Pipeline = GraphicsPipeline::Create(_info.pTarget->GetContext());
		m_Pipeline->SetShaderStages(v);
		m_Pipeline->SetVertexInputLayout({});
		m_Pipeline->SetRasterizationSettings({});
		m_Pipeline->SetColorBlendSettings({});
		m_Pipeline->SetRenderPass(m_RenderPass);
		m_Pipeline->Build();
		// =========

		create_frame_buffers();

		// Start the render thread
		m_RenderCmdChain.Start();
	}

	void Renderer2D::Shutdown()
	{
		// Stop the render thread
		m_RenderCmdChain.Stop();

		//m_Pipeline->Destroy();
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

	void Renderer2D::create_frame_buffers()
	{
		ANV_PROFILE_SCOPE()
		m_FrameBuffers.resize(m_RenderCreateInfo.swapchainImageCount);
		auto image_views = m_RenderCreateInfo.pTarget->GetContext()->GetSwapchain()->GetImageViews();
		for (int i = 0; i < image_views.size(); i++)
		{
			m_FrameBuffers[i] = Framebuffer::Create(m_RenderCreateInfo.pTarget->GetContext(), image_views[i], m_RenderPass);
		}
	}
}
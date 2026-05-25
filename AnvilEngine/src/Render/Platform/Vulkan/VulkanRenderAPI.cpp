#include "VulkanRenderAPI.h"
#include "Core/App.h"
#include "Core/Window.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include <Util/Time/Time.h>
#include "VulkanBuffer.h"
#include "Render/Vertex.h"
#include "VulkanPipeline.h"

namespace anv {

	VulkanRenderAPI::VulkanRenderAPI(Render2DCreateInfo _info)
		: m_CreateInfo(_info)
	{
		ANV_PROFILE_SCOPE();

		// TODO: should probably be done in the parent class. all children of RenderAPI will use these
		m_Context = m_CreateInfo.pTarget->GetContext();
		m_AssetManager = App::GetInstance()->GetAssetManager();
		m_RenderCmdChain = std::make_shared<QueueChain>();

		m_RenderCmdChain->Start();

		create_render_passes();
		load_shader_lib();
		create_quad_buffers();

		// create camera UBO
		BufferCreateInfo camera_info{};
		camera_info.Usage = BufferUsage::Uniform;
		camera_info.Size = sizeof(CameraUBO);
		camera_info.Dynamic = true;
		m_CameraUBO = Buffer::Create(m_Context, camera_info);
		
		create_descriptor_set_layout();
		create_descriptor_pool();
		create_camera_descriptor_set();

		build_2D_pipelines();
		create_frame_buffers();
		create_frames();

		m_RenderPass.As<VulkanRenderPass>()->SetFramebuffers(m_FrameBuffers);
	}

	void VulkanRenderAPI::DrawFrame()
	{
		if (m_RecreatingSwapchain.load(std::memory_order_relaxed))
			return;

		auto swap = m_Context->GetSwapchain().As<VulkanSwapchain>();
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		auto& fr = m_Frames[m_FrameIndex];

		// wait/reset fence
		vkWaitForFences(m_Context->GetAs<VulkanContext>()->GetDevice(), 1, &fr.sync.inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Context->GetAs<VulkanContext>()->GetDevice(), 1, &fr.sync.inFlightFence);

		// acquire
		uint32_t imageIndex = m_Context->GetAs<VulkanContext>()->GetSwapchain()->AcquireNextImage(fr.sync.imageAvailable, m_SwapRecreateFlag, VK_NULL_HANDLE);
		
		if (m_SwapRecreateFlag)
		{
			ANV_LOG_INFO("SwapChain Recreation due to winow resize")
			recreate_swap();
			m_SwapRecreateFlag = false;
			return;
		}

		// set active cmd + frame context (engine-level)
		SwapExtent ext = m_Context->GetAs<VulkanContext>()->GetSwapchain()->GetExtent();
		RenderFrameContext frame{ imageIndex, ext.width, ext.height };
		
		// Bind the active frame to the render thread
		fr.cmd->Reset();
		m_RenderCmdChain->SetActiveCommandBuffer(fr.cmd);
		m_RenderCmdChain->SetActiveFrame(frame);
		// IMPORTANT: also tell the submit path which frame sync to use
		m_RenderCmdChain->SetActiveFrameSyncIndex(m_FrameIndex); // or store directly on QueueChain

		uint32_t frameIdx = m_FrameIndex;

		m_RenderCmdChain->SetSubmitFn([this, frameIdx](Ref<CommandBuffer> cmd)
			{
				auto& fr2 = m_Frames[frameIdx];

				auto vkCmd = cmd.As<VulkanCommandBuffer>();
				ANV_ASSERT(vkCmd, "Submit: CommandBuffer cast failed!");

				vkCmd->Submit(
					fr2.sync.imageAvailable,
					fr2.sync.renderFinished,
					fr2.sync.inFlightFence);
			});

		m_SpritePipeline->Bind(m_RenderCmdChain);
		m_RenderPass->Begin();

		m_RenderCmdChain->WriteToBack([=](Ref<CommandBuffer> cmd, const RenderFrameContext& frame)
		{
				auto vkCmd = cmd.As<VulkanCommandBuffer>();
				auto ext = m_Context->GetSwapchain()->GetExtent();

				VkViewport vp{};
				vp.x = 0.0f;
				vp.y = 0.0f;
				vp.width = (float)ext.width;
				vp.height = (float)ext.height;
				vp.minDepth = 0.0f;
				vp.maxDepth = 1.0f;
				vkCmdSetViewport(vkCmd->Get(), 0, 1, &vp);

				VkRect2D sc{};
				sc.offset = { 0, 0 };
				sc.extent = { ext.width, ext.height };
				vkCmdSetScissor(vkCmd->Get(), 0, 1, &sc);

				// drawing the quad
				auto vkVB = m_QuadVB.As<VulkanBuffer>();
				auto vkIB = m_QuadIB.As<VulkanBuffer>();

				VkBuffer vertexBuffers[] = { vkVB->GetBuffer()};
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindDescriptorSets(
					vkCmd->Get(),
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_SpritePipeline.As<VulkanPipeline>()->GetPipelineLayout(),
					0,
					1,
					&m_CameraDescriptorSet,
					0,
					nullptr
				);

				// bind buffers
				vkCmdBindVertexBuffers(
					vkCmd->Get(),
					0,
					1,
					vertexBuffers,
					offsets
				);

				vkCmdBindIndexBuffer(
					vkCmd->Get(),
					vkIB->GetBuffer(),
					0,
					VK_INDEX_TYPE_UINT32
				);

				//draw
				vkCmdDrawIndexed(
					vkCmd->Get(),
					6,
					1,
					0,
					0,
					0
				);

				//vkCmdDraw(cmd.As<VulkanCommandBuffer>()->Get(), 3, 1, 0, 0);
		});

		m_RenderPass->End();

		m_RenderCmdChain->Swap();
		m_RenderCmdChain->WaitForProcessComplete();

		// present waits on renderFinished
		swap->Present(vkCtx->GetPresentQueue(), imageIndex, fr.sync.renderFinished, m_SwapRecreateFlag);
		if (m_SwapRecreateFlag)
		{
			recreate_swap();
			m_SwapRecreateFlag = false;
			return;
		}

		m_FrameIndex = (m_FrameIndex + 1) % (uint32_t)m_Frames.size();
	}


	void VulkanRenderAPI::OnShutdown()
	{
		m_Context->GetAs<VulkanContext>()->IdleDevice();
		destroy_frames();
	}

	void anv::VulkanRenderAPI::BeginScene()
	{
		// Reset rendering statistics
		// Reset tmp frame data
	
		ANV_ASSERT(m_Camera, "Renderer2D has no main camera set!");

		m_Camera->Update(Time::DeltaTime());

		// Upload CameraUBO
		m_CameraUBO->SetData(
			&m_Camera->GetCameraUBO(),
			sizeof(CameraUBO)
		);
	}

	void VulkanRenderAPI::DrawQuad(const glm::vec2& position, const glm::vec2& size, glm::vec4 color)
	{
	}

	void VulkanRenderAPI::EndScene()
	{
	}

	void VulkanRenderAPI::SetMainCamera(_shared<Camera2D> camera)
	{
		m_Camera = camera;
	}

	void VulkanRenderAPI::create_render_passes()
	{
		RenderPassCreateInfo rpinfo{};
		rpinfo.d_name = "GeometryPass";

		rpinfo.commandQueue = m_RenderCmdChain;

		RenderPass::Attachment colatt;
		colatt.type = RenderPass::Attachment::Type::ATT_TY_COLOR;
		colatt.loadOp = RenderPass::Attachment::LoadOp::LOAD_OP_CLEAR;
		colatt.storeOp = RenderPass::Attachment::StoreOp::STORE_OP_STORE;
		colatt.beginLayout = RenderPass::Attachment::ImgLayout::IMG_LAYOUT_UNDEF;
		colatt.endLayout = RenderPass::Attachment::ImgLayout::IMG_LAYOUT_PRES;
		rpinfo.attachments.push_back(colatt);

		RenderPassCreateInfo::SubpassInfo rpspinfo{
			.colorAttachments = {0},     // ref the first color attach
			.depthStencilAttachment = -1 // no depth att
		};

		rpinfo.subpasses.push_back(rpspinfo);

		m_RenderPass = RenderPass::Create(rpinfo, m_CreateInfo.pTarget->GetContext());
		m_RenderPass->Build();
	}

	void VulkanRenderAPI::load_shader_lib()
	{
		// TODO: need to update this when we actually have more shaders
		//auto path = App::GetInstance()->GetFS().GetKeyVal("ShaderLib") / "shader.glsl";
		//m_Shader = m_AssetManager->CreateShader(path.string(), m_CreateInfo.pTarget->GetContext());
		// sprite
		auto spritepath = App::GetInstance()->GetFS().GetKeyVal("ShaderLib") / "sprite.glsl";
		m_SpriteShader = m_AssetManager->CreateShader(spritepath.string(), m_CreateInfo.pTarget->GetContext());
	}

	void VulkanRenderAPI::build_2D_pipelines()
	{
		// Basic pipeline (triangle)
		//m_Pipeline = m_AssetManager->CreateGraphicsPipeline(m_CreateInfo.pTarget->GetContext(),
		//	"Test Pipeline");
		//m_Pipeline->SetShaderStages(m_Shader);
		//m_Pipeline->SetVertexInputLayout({});
		//m_Pipeline->SetRasterizationSettings({});
		//m_Pipeline->SetColorBlendSettings({});
		//m_Pipeline->SetRenderPass(m_RenderPass);
		//m_Pipeline->Build();

		VertexInputLayout sprite_layout{};

		// Sprite Pipeline
		VertexInputLayout quadLayout{};
		quadLayout.binding = 0;
		quadLayout.stride = sizeof(QuadVertex);

		quadLayout.AddAttribute(
			"Position",
			0,
			offsetof(QuadVertex, Position),
			sizeof(glm::vec2),
			sizeof(QuadVertex)
		);

		quadLayout.AddAttribute(
			"Color",
			1,
			offsetof(QuadVertex, Color),
			sizeof(glm::vec4),
			sizeof(QuadVertex)
		);

		m_SpritePipeline =
			m_AssetManager->CreateGraphicsPipeline(
				m_Context,
				"Sprite Pipeline"
			);

		m_SpritePipeline->SetShaderStages(m_SpriteShader);
		m_SpritePipeline->SetVertexInputLayout(&quadLayout);
		m_SpritePipeline->SetRasterizationSettings(nullptr);
		m_SpritePipeline.As<VulkanPipeline>()->SetDescriptorSetLayouts({
			m_CameraDescriptorSetLayout
		});
		m_SpritePipeline->SetColorBlendSettings(nullptr);
		m_SpritePipeline->SetRenderPass(m_RenderPass);
		m_SpritePipeline->Build();
		// Post Processing
	}

	void VulkanRenderAPI::create_frames()
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		VkDevice device = vkCtx->GetDevice();

		m_Frames.resize(m_CreateInfo.swapchainImageCount);

		VkSemaphoreCreateInfo semInfo{};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // IMPORTANT so first frame doesn't stall

		for (uint32_t i = 0; i < m_CreateInfo.swapchainImageCount; i++)
		{
			// --- sync ---
			ANV_VK_CHECK_RESULT(vkCreateSemaphore(device, &semInfo, nullptr, &m_Frames[i].sync.imageAvailable),
				"Failed to create imageAvailable semaphore!");
			ANV_VK_CHECK_RESULT(vkCreateSemaphore(device, &semInfo, nullptr, &m_Frames[i].sync.renderFinished),
				"Failed to create renderFinished semaphore!");
			ANV_VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &m_Frames[i].sync.inFlightFence),
				"Failed to create inFlight fence!");

			m_Frames[i].cmd = Ref<VulkanCommandBuffer>::Create(m_Context);
		}

		m_FrameIndex = 0;
	}

	void VulkanRenderAPI::destroy_frames()
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		VkDevice device = vkCtx->GetDevice();

		for (auto& fr : m_Frames)
		{
			if (fr.sync.imageAvailable)
				vkDestroySemaphore(device, fr.sync.imageAvailable, nullptr);
			if (fr.sync.renderFinished)
				vkDestroySemaphore(device, fr.sync.renderFinished, nullptr);
			if (fr.sync.inFlightFence)
				vkDestroyFence(device, fr.sync.inFlightFence, nullptr);

			fr.sync.imageAvailable = VK_NULL_HANDLE;
			fr.sync.renderFinished = VK_NULL_HANDLE;
			fr.sync.inFlightFence = VK_NULL_HANDLE;

			fr.cmd = nullptr; // let Ref cleanup
		}

		m_Frames.clear();
	}

	void VulkanRenderAPI::create_frame_buffers()
	{
		ANV_PROFILE_SCOPE();

		auto ctx = m_CreateInfo.pTarget->GetContext();
		auto views = ctx->GetSwapchain()->GetImageViews();

		m_FrameBuffers.resize(views.size());

		for (size_t i = 0; i < views.size(); i++)
			m_FrameBuffers[i] = Framebuffer::Create(ctx, views[i], m_RenderPass);
	}

	void VulkanRenderAPI::recreate_swap()
	{
		m_RecreatingSwapchain.store(true);

		m_RenderCmdChain->Flush();
		
		m_Context->GetAs<VulkanContext>()->IdleDevice();

		for (size_t i = 0; i < m_FrameBuffers.size(); i++)
		{
			m_FrameBuffers[i].Reset();
		}
		m_RenderPass.Reset();
		
		m_Context->GetSwapchain()->ResetSwap();

		create_render_passes();
		create_frame_buffers();
		m_RenderPass.As<VulkanRenderPass>()->SetFramebuffers(m_FrameBuffers);

		m_RecreatingSwapchain.store(false);
	}

	void VulkanRenderAPI::create_quad_buffers()
	{
		ANV_LOG_INFO("IB size: {}", sizeof(Quad::indices));
		ANV_LOG_INFO("VB size: {}", sizeof(Quad::vertices));

		BufferCreateInfo vbi{};
		vbi.Usage = BufferUsage::Vertex;
		vbi.Size = sizeof(Quad::vertices);
		vbi.InitialData = Quad::vertices;
		m_QuadVB = Buffer::Create(m_Context, vbi);

		BufferCreateInfo ibi{};
		ibi.Usage = BufferUsage::Index;
		ibi.Size = sizeof(Quad::indices);
		ibi.InitialData = Quad::indices;
		m_QuadIB = Buffer::Create(m_Context, ibi);
	}
	void VulkanRenderAPI::create_descriptor_set_layout()
	{
		VkDescriptorSetLayoutBinding cameraBinding{};
		cameraBinding.binding = 0;
		cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraBinding.descriptorCount = 1;
		cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		cameraBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &cameraBinding;

		ANV_VK_CHECK_RESULT(
			vkCreateDescriptorSetLayout(
				m_Context->GetAs<VulkanContext>()->GetDevice(),
				&layoutInfo,
				nullptr,
				&m_CameraDescriptorSetLayout
			),
			"Failed to create camera descriptor set layout!"
		);
	}
	void VulkanRenderAPI::create_descriptor_pool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 1;

		ANV_VK_CHECK_RESULT(
			vkCreateDescriptorPool(
				m_Context->GetAs<VulkanContext>()->GetDevice(),
				&poolInfo,
				nullptr,
				&m_DescriptorPool
			),
			"Failed to create descriptor pool!"
		);
	}
	void VulkanRenderAPI::create_camera_descriptor_set()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_CameraDescriptorSetLayout;

		ANV_VK_CHECK_RESULT(
			vkAllocateDescriptorSets(
				m_Context->GetAs<VulkanContext>()->GetDevice(),
				&allocInfo,
				&m_CameraDescriptorSet
			),
			"Failed to allocate camera descriptor set!"
		);

		auto vkUBO = m_CameraUBO.As<VulkanBuffer>();

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vkUBO->GetBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraUBO);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_CameraDescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(
			m_Context->GetAs<VulkanContext>()->GetDevice(),
			1,
			&descriptorWrite,
			0,
			nullptr
		);
	}
}
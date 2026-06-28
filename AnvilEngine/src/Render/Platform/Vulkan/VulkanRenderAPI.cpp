#include "VulkanRenderAPI.h"
#include "Core/App.h"
#include "Core/Window.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "Render/RenderData.h"
#include "VulkanPipeline.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <Util/Time/Time.h>
#include <glm/ext/matrix_transform.hpp>

namespace anv {

	struct SpritePush
	{
		glm::mat4 Model;
		glm::vec4 Color;
	};

	VulkanRenderAPI::VulkanRenderAPI(Render2DCreateInfo _info)
		: m_CreateInfo(_info)
	{
		ANV_PROFILE_SCOPE();

		// TODO: should probably be done in the parent class. all children of RenderAPI will use these
		m_Context = m_CreateInfo.pTarget->GetContext();
		m_AssetManager = App::GetInstance()->GetAssetManager();
		m_RenderCmdChain = std::make_shared<QueueChain>();

		m_RenderCmdChain->Start();

		load_shader_lib();
		create_quad_buffers();

		// create camera UBO
		BufferCreateInfo camera_info{};
		camera_info.Usage = BufferUsage::Uniform;
		camera_info.Size = sizeof(CameraUBO);
		camera_info.Dynamic = true;
		m_CameraUBO = Buffer::Create(m_Context, camera_info);
		
		create_imgui_descriptor_pool();
		create_descriptor_set_layout();
		create_descriptor_pool();
		create_camera_descriptor_set();

		m_SwapchainTarget = Ref<VulkanSwapchainRenderTarget>::Create(m_Context);
		m_CurrentTarget = m_SwapchainTarget;

		build_2D_pipelines();
		create_frames();

		init_imgui();
	}

	void VulkanRenderAPI::DrawFrame()
	{
		if (m_RecreatingSwapchain.load(std::memory_order_relaxed))
			return;

		auto swap = m_Context->GetSwapchain().As<VulkanSwapchain>();
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		auto& fr = m_Frames[m_FrameIndex];

		// wait/reset fence
		vkWaitForFences(m_Context->GetAs<VulkanContext>()->
			GetDevice(), 1, &fr.sync.inFlightFence, VK_TRUE, UINT64_MAX);

		vkResetFences(m_Context->GetAs<VulkanContext>()->
			GetDevice(), 1, &fr.sync.inFlightFence);

		// acquire
		uint32_t imageIndex = m_SwapchainTarget->AcquireNextImage(fr.sync.imageAvailable, m_SwapRecreateFlag);

		if (m_SwapRecreateFlag)
		{
			ANV_LOG_INFO("SwapChain Recreation due to winow resize")
			recreate_swap();
			m_SwapRecreateFlag = false;
			return;
		}

		// set active cmd + frame context (engine-level)
		SwapExtent ext = m_Context->GetAs<VulkanContext>()->
			GetSwapchain()->GetExtent();
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

		
		m_RenderCmdChain->WriteToBack([=](Ref<CommandBuffer> cmd, const RenderFrameContext& frame)
		{
				// all scene logic moved to DrawScene()
				m_SwapchainTarget->Begin(cmd);
				end_imgui(cmd);
				m_SwapchainTarget->End(cmd);
		});

		m_RenderCmdChain->Swap();
		m_RenderCmdChain->WaitForProcessComplete();

		// update after main render loop
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// present waits on renderFinished
		m_SwapchainTarget->Present(
			vkCtx->GetPresentQueue(),
			fr.sync.renderFinished,
			m_SwapRecreateFlag
		);

		if (m_SwapRecreateFlag)
		{
			recreate_swap();
			m_SwapchainTarget->Resize(
				vkCtx->GetSwapchain()->GetExtent().width,
				vkCtx->GetSwapchain()->GetExtent().height
			);

			m_SwapRecreateFlag = false;

			// update aspect ratio
			uint32_t width = m_CurrentTarget->GetWidth();
			uint32_t height = m_CurrentTarget->GetHeight();

			float ar =
				static_cast<float>(width) /
				static_cast<float>(height);

			if (m_Camera)
				m_Camera->SetAspectRatio(ar);
			return;
		}
		m_FrameIndex = (m_FrameIndex + 1) % (uint32_t)m_Frames.size();
	}


	void VulkanRenderAPI::OnShutdown()
	{
		m_Context->GetAs<VulkanContext>()->IdleDevice();
		destroy_frames();
		shutdown_imgui();
	}

	void anv::VulkanRenderAPI::BeginScene(Ref<RenderTarget> _renderTarget)
	{
		// Reset rendering statistics
		m_QuadQueue.clear();
		m_RenderStats.QuadCount = 0;
		// Reset tmp frame data
	
		ANV_ASSERT(m_CurrentTarget, "No RenderTarget specifide for Renderer2D!");

		m_CurrentTarget = _renderTarget;

		ANV_ASSERT(m_Camera, "Renderer2D has no main camera set!");

		m_Camera->Update(Time::DeltaTime());

		// Upload CameraUBO
		m_CameraUBO->SetData(
			&m_Camera->GetCameraUBO(),
			sizeof(CameraUBO)
		);

		begin_imgui();
	}

	void VulkanRenderAPI::BeginScene()
	{
		BeginScene(m_SwapchainTarget);
	}

	void VulkanRenderAPI::DrawScene(Ref<RenderTarget> _renderTarget)
	{
		auto pipeline = m_PipelineLibrary.Get("Sprite", _renderTarget);
		m_RenderCmdChain->WriteToBack([=](Ref<CommandBuffer> cmd, const RenderFrameContext& frame) mutable
		{
				auto stableiz_pipeline = pipeline;
				auto stableize_rt = _renderTarget;
				auto vkCmd = cmd.As<VulkanCommandBuffer>();

				stableize_rt->Begin(cmd);
				stableiz_pipeline->Bind(cmd);

				VkViewport vp{};
				vp.x = 0.0f;
				vp.y = 0.0f;
				vp.width = (float)_renderTarget->GetWidth();
				vp.height = (float)_renderTarget->GetHeight();
				vp.minDepth = 0.0f;
				vp.maxDepth = 1.0f;
				vkCmdSetViewport(vkCmd->Get(), 0, 1, &vp);

				VkRect2D sc{};
				sc.offset = { 0, 0 };
				sc.extent = { _renderTarget->GetWidth(), _renderTarget->GetHeight() };
				vkCmdSetScissor(vkCmd->Get(), 0, 1, &sc);

				// drawing the quad
				auto vkVB = m_QuadVB.As<VulkanBuffer>();
				auto vkIB = m_QuadIB.As<VulkanBuffer>();

				VkBuffer vertexBuffers[] = { vkVB->GetBuffer() };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindDescriptorSets(
					vkCmd->Get(),
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline.As<VulkanPipeline>()->GetPipelineLayout(),
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

				m_RenderStats.DrawCalls = 0;

				// draw quads
				for (auto& quad : m_QuadQueue)
				{
					SpritePush push{};

					push.Model =
						glm::translate(
							glm::mat4(1.0f),
							glm::vec3(quad.Position, 0)
						)
						*
						glm::scale(
							glm::mat4(1.0f),
							glm::vec3(quad.Size, 1)
						);

					push.Color = quad.Color;

					// update push data
					vkCmdPushConstants(
						vkCmd->Get(),
						pipeline.As<VulkanPipeline>()
						->GetPipelineLayout(),
						VK_SHADER_STAGE_VERTEX_BIT,
						0,
						sizeof(SpritePush),
						&push
					);

					// draw
					vkCmdDrawIndexed(
						vkCmd->Get(),
						6,
						1,
						0,
						0,
						0
					);
					m_RenderStats.DrawCalls++;
				}
				_renderTarget->End(cmd);
		});
	}

	void VulkanRenderAPI::DrawQuad(const glm::vec2& position, const glm::vec2& size, glm::vec4 color)
	{
		m_QuadQueue.push_back({
			position,
			size,
			color
		});
		m_RenderStats.QuadCount++;
	}

	void VulkanRenderAPI::EndScene()
	{
		
	}

	void VulkanRenderAPI::SetMainCamera(_shared<Camera2D> camera)
	{
		m_Camera = camera;
	}

	RendererStats VulkanRenderAPI::GetStats()
	{
		return m_RenderStats;
	}

	void VulkanRenderAPI::load_shader_lib()
	{
		// sprite
		auto spritepath = App::GetInstance()->GetFS().GetKeyVal("ShaderLib") / "sprite.glsl";
		m_SpriteShader = m_AssetManager->CreateShader(spritepath.string(), m_CreateInfo.pTarget->GetContext());
	}

	void VulkanRenderAPI::build_2D_pipelines()
	{
		m_PipelineLibrary.Register(
			"Sprite",
			[this](Ref<RenderPass> renderPass)
			{
				return build_sprite_pipeline(renderPass);
			}
		);
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

	void VulkanRenderAPI::recreate_swap()
	{
		m_RecreatingSwapchain.store(true);

		m_RenderCmdChain->Flush();
		
		m_Context->GetAs<VulkanContext>()->IdleDevice();

		m_Context->GetSwapchain()->ResetSwap();

		m_RecreatingSwapchain.store(false);

		// all render passes in render targets are now old. they also will be recreated
		m_PipelineLibrary.Clear();
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

	void VulkanRenderAPI::begin_batch()
	{
	}

	void VulkanRenderAPI::end_batch()
	{
	}

	void VulkanRenderAPI::create_imgui_descriptor_pool()
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000 * std::size(poolSizes);
		poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
		poolInfo.pPoolSizes = poolSizes;

		ANV_VK_CHECK_RESULT(
			vkCreateDescriptorPool(
				m_Context->GetAs<VulkanContext>()->GetDevice(),
				&poolInfo,
				nullptr,
				&m_ImGuiDescriptorPool
			),
			"Failed to create ImGui descriptor pool!"
		);
	}

	void VulkanRenderAPI::init_imgui()
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGuiStyle& style = ImGui::GetStyle();

			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		auto vkCtx = m_Context->GetAs<VulkanContext>();

		ImGui_ImplGlfw_InitForVulkan(
			App::GetInstance()->GetMainWindow()->GetNativeWindow(),
			true
		);

		ImGui_ImplVulkan_InitInfo init{};
		init.ApiVersion = VK_API_VERSION_1_3; 
		init.Instance = vkCtx->GetInstance();
		init.PhysicalDevice = vkCtx->GetPhysicalDevice();
		init.Device = vkCtx->GetDevice();
		init.QueueFamily = vkCtx->GetQueueFamilies().graphicsFamily.value();
		init.Queue = vkCtx->GetGraphicsQueue();

		init.DescriptorPool = m_ImGuiDescriptorPool;
		// OR use automatic pool:
		// init.DescriptorPoolSize = 1000;

		init.MinImageCount = vkCtx->GetSwapchain()->GetImageCount();
		init.ImageCount = vkCtx->GetSwapchain()->GetImageCount();
		init.PipelineCache = VK_NULL_HANDLE;

		// IMPORTANT
		init.PipelineInfoMain.RenderPass =
			m_SwapchainTarget->GetRenderPass().As<VulkanRenderPass>()->Get();

		init.PipelineInfoMain.Subpass = 0;

		init.PipelineInfoMain.MSAASamples =
			VK_SAMPLE_COUNT_1_BIT;

		init.UseDynamicRendering = false;
		init.Allocator = nullptr;
		init.CheckVkResultFn = nullptr;
		init.MinAllocationSize = 1024 * 1024;

		ImGui_ImplVulkan_Init(&init);

		ANV_LOG_INFO("Initialized ImGui");
	}

	void VulkanRenderAPI::shutdown_imgui()
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();

		vkCtx->IdleDevice();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();

		vkDestroyDescriptorPool(
			m_Context->GetAs<VulkanContext>()->GetDevice(),
			m_ImGuiDescriptorPool,
			nullptr
		);

		ANV_LOG_INFO("Shutdown ImGui");
	}
	void VulkanRenderAPI::begin_imgui()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
	}
	void VulkanRenderAPI::end_imgui(Ref<CommandBuffer> cmd)
	{
		ImGui::Render();

		auto vkCmd = cmd.As<VulkanCommandBuffer>();

		ImGui_ImplVulkan_RenderDrawData(
			ImGui::GetDrawData(),
			vkCmd->Get()
		);
	}

	Ref<GraphicsPipeline> VulkanRenderAPI::build_sprite_pipeline(
		Ref<RenderPass> renderPass)
	{
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

		auto pipeline = m_AssetManager->CreateGraphicsPipeline(
			m_Context,
			"Sprite Pipeline"
		);

		pipeline->SetShaderStages(m_SpriteShader);
		pipeline->SetVertexInputLayout(&quadLayout);
		pipeline->SetRasterizationSettings(nullptr);

		pipeline.As<VulkanPipeline>()->SetDescriptorSetLayouts({
			m_CameraDescriptorSetLayout
			});

		pipeline.As<VulkanPipeline>()->SetPushConstantRange(
			VK_SHADER_STAGE_VERTEX_BIT,
			sizeof(SpritePush)
		);

		pipeline->SetColorBlendSettings(nullptr);
		pipeline->SetRenderPass(renderPass);
		pipeline->Build();

		return pipeline;
	}
}
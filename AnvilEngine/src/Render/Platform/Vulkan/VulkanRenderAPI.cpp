#include "VulkanRenderAPI.h"
#include "Core/App.h"
#include "Core/Window.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "Render/RenderData.h"
#include "VulkanPipeline.h"

#include <Util/Time/Time.h>
#include <glm/ext/matrix_transform.hpp>
#include <cmath>

namespace anv
{

	struct SpritePush
	{
		glm::mat4 Transform;
		glm::vec4 Color;
	};

	VulkanRenderAPI::VulkanRenderAPI(Render2DCreateInfo _info)
		: m_CreateInfo(_info)
	{
		ANV_PROFILE_SCOPE();
		m_Context = m_CreateInfo.pTarget->GetContext();
		m_RenderCmdChain = std::make_shared<QueueChain>();
		m_RenderCmdChain->Start();
		load_shader_lib();
		create_quad_buffers();

		create_descriptor_set_layout();
		create_white_texture();

		m_SwapchainTarget = Ref<VulkanSwapchainRenderTarget>::Create(m_Context);
		m_CurrentTarget = m_SwapchainTarget;
		build_2D_pipelines();
		create_frames();
		m_ImGuiLayer = ImGuiLayer::Create(m_Context, m_SwapchainTarget, m_CreateInfo.imguiIniPath);
		ANV_ASSERT(m_ImGuiLayer, "Failed to initialize ImGui layer");
	}

	void VulkanRenderAPI::DrawFrame()
	{
		if (m_RecreatingSwapchain.load(std::memory_order_relaxed))
			return;

		auto vkCtx = m_Context->GetAs<VulkanContext>();
		auto &fr = m_Frames[m_FrameIndex];

        {
            ANV_PROFILE_ZONE(GPUFenceWait);
            vkWaitForFences(vkCtx->GetDevice(), 1, &fr.sync.inFlightFence, VK_TRUE, UINT64_MAX);
        }

		m_FrameVertexOffsets[m_FrameIndex] = 0;

		BufferCreateInfo vbInfo{};
		vbInfo.Usage = BufferUsage::Vertex;
		vbInfo.Size =
			MaxBatchQuads * 4 * sizeof(BatchVertex);
		vbInfo.Dynamic = true;

        uint32_t imageIndex = 0;
        {
            ANV_PROFILE_ZONE(SwapchainAcquire);
            imageIndex = m_SwapchainTarget->AcquireNextImage(fr.sync.imageAvailable, m_SwapRecreateFlag);
        }
		if (m_SwapRecreateFlag)
		{
			ANV_LOG_INFO("SwapChain recreation due to window resize")
			recreate_swap();
			m_SwapRecreateFlag = false;
			return;
		}

		m_FrameVertexOffsets[m_FrameIndex] = 0;

		vkResetFences(vkCtx->GetDevice(), 1, &fr.sync.inFlightFence);

		SwapExtent ext = vkCtx->GetSwapchain()->GetExtent();
		RenderFrameContext frame{imageIndex, ext.width, ext.height};
		fr.cmd->Reset();
		m_RenderCmdChain->SetActiveCommandBuffer(fr.cmd);
		m_RenderCmdChain->SetActiveFrame(frame);
		m_RenderCmdChain->SetActiveFrameSyncIndex(m_FrameIndex);

		uint32_t frameIdx = m_FrameIndex;
		m_RenderCmdChain->SetSubmitFn([this, frameIdx](Ref<CommandBuffer> cmd)
									  {
			auto& fr2 = m_Frames[frameIdx];
			auto vkCmd = cmd.As<VulkanCommandBuffer>();
			ANV_ASSERT(vkCmd, "Submit: CommandBuffer cast failed!");
			vkCmd->Submit(fr2.sync.imageAvailable, fr2.sync.renderFinished, fr2.sync.inFlightFence); });

		m_RenderCmdChain->WriteToBack([=](Ref<CommandBuffer> cmd, const RenderFrameContext &frame)
									  {
			m_SwapchainTarget->Begin(cmd);
			m_ImGuiLayer->Render({.commandBuffer = cmd});
			m_SwapchainTarget->End(cmd); });

        {
            ANV_PROFILE_ZONE(RenderSubmitWait);
            {
                ANV_PROFILE_ZONE(RenderQueueSwap);
                m_RenderCmdChain->Swap();
            }
            {
                ANV_PROFILE_ZONE(RenderWorkerWait);
                m_RenderCmdChain->WaitForProcessComplete();
            }
            // Read published worker timings only after the completion signal.
            const auto worker = m_RenderCmdChain->GetWorkerTimings();
            FrameProfiler::Record(FrameProfiler::Zone::WorkerBegin, worker.begin);
            FrameProfiler::Record(FrameProfiler::Zone::WorkerTasks, worker.tasks);
            FrameProfiler::Record(FrameProfiler::Zone::WorkerEnd, worker.end);
            FrameProfiler::Record(FrameProfiler::Zone::WorkerSubmit, worker.submit);
            FrameProfiler::Record(FrameProfiler::Zone::WorkerTotal, worker.total);
            const auto sprites = m_RenderCmdChain->GetSpriteTaskTimings();
            FrameProfiler::Record(FrameProfiler::Zone::SpriteSort, sprites.sort);
            FrameProfiler::Record(FrameProfiler::Zone::SpriteBuild, sprites.build);
            FrameProfiler::Record(FrameProfiler::Zone::SpriteUpload, sprites.upload);
            FrameProfiler::Record(FrameProfiler::Zone::SpriteCommands, sprites.commands);
            {
                ANV_PROFILE_ZONE(ImGuiFinish);
                m_ImGuiLayer->FinishFrame();
            }
        }

        {
            ANV_PROFILE_ZONE(Present);
            m_SwapchainTarget->Present(vkCtx->GetPresentQueue(), fr.sync.renderFinished, m_SwapRecreateFlag);
        }
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
		auto vkctx = m_Context->GetAs<VulkanContext>();
		vkctx->IdleDevice();

		if (m_RenderCmdChain)
		{
			m_RenderCmdChain->Flush();
			m_RenderCmdChain->Stop();
		}

		m_PipelineLibrary.Clear();
		m_WhiteTexture = nullptr;
		destroy_descriptor_resources();
		destroy_frames();
		m_ImGuiLayer.reset();
	}

	void anv::VulkanRenderAPI::BeginScene(Ref<RenderTarget> _renderTarget)
	{
		m_QuadQueue.clear();
		m_RenderStats.QuadCount = 0;
		ANV_ASSERT(_renderTarget, "No RenderTarget specified for Renderer2D!");
		m_CurrentTarget = _renderTarget;
		m_ImGuiLayer->BeginFrame();
	}

	void VulkanRenderAPI::BeginScene()
	{
		BeginScene(m_SwapchainTarget);
	}

	void VulkanRenderAPI::DrawScene(
		Ref<RenderTarget> _renderTarget,
		_shared<Camera2D> camera)
	{
		ANV_ASSERT(_renderTarget, "DrawScene requires a render target");
		ANV_ASSERT(camera, "DrawScene requires a camera");

		camera->Update(Time::DeltaTime());

		const glm::mat4 sceneViewProjection =
			camera->GetCameraUBO().ViewProjection;

		auto sceneQuads = m_QuadQueue;
		auto pipeline = m_PipelineLibrary.Get("Sprite", _renderTarget);

		m_RenderStats.QuadCount = sceneQuads.size();

		m_RenderCmdChain->WriteToBack(
			[=, this](Ref<CommandBuffer> cmd,
					  const RenderFrameContext &frame) mutable
			{
                using SpriteClock = std::chrono::steady_clock;
                const auto spriteStart = SpriteClock::now();
                QueueChain::SpriteTaskTimings spriteTimes{};
                auto elapsed = [](SpriteClock::time_point start) {
                    return std::chrono::duration<float, std::milli>(
                        SpriteClock::now() - start).count();
                };
				// Preserve layer ordering and same-layer submission order.
				std::stable_sort(
					sceneQuads.begin(),
					sceneQuads.end(),
					[](const QuadSubmission &a, const QuadSubmission &b)
					{
						return a.Layer < b.Layer;
					});

                spriteTimes.sort = elapsed(spriteStart);
                const auto buildStart = SpriteClock::now();

				struct Batch
				{
					Ref<VulkanTexture> Texture;
					uint32_t FirstQuad = 0;
					uint32_t QuadCount = 0;
				};

				// Only the render worker touches these buffers. Preserve capacity
				// between frames instead of allocating on every DrawScene task.
				static thread_local std::vector<BatchVertex> vertices;
				static thread_local std::vector<Batch> batches;
				vertices.clear();
				batches.clear();
				vertices.resize(sceneQuads.size() * 4);
				batches.reserve(sceneQuads.size());

				const glm::vec2 uv[4] = {
					{0.0f, 0.0f}, {1.0f, 0.0f},
					{1.0f, 1.0f}, {0.0f, 1.0f}};

				// ---- CPU batching ----

				std::size_t quadIndex = 0;
				for (const auto &quad : sceneQuads)
				{
					Ref<VulkanTexture> texture = m_WhiteTexture;

					if (quad.TextureAsset)
					{
						auto requested =
							quad.TextureAsset.As<VulkanTexture>();

						if (requested && requested->IsGPUReady())
							texture = requested;
					}

					ANV_ASSERT(
						texture && texture->IsGPUReady(),
						"No valid Vulkan sprite texture available");

					// Only merge adjacent sprites with the same texture.
					if (batches.empty() ||
						batches.back().Texture != texture)
					{
						Batch batch{};
						batch.Texture = texture;
						batch.FirstQuad =
							static_cast<uint32_t>(quadIndex);

						batches.push_back(batch);
					}

					// Compute the four corners directly in 2D. A 4x4
					// translate/rotate/scale matrix is unnecessary for quads.
					const float halfX = quad.Size.x * 0.5f;
					const float halfY = quad.Size.y * 0.5f;
					const float angle = glm::radians(quad.Rotation);
					const float c = quad.Rotation == 0.0f ? 1.0f : std::cos(angle);
					const float sn = quad.Rotation == 0.0f ? 0.0f : std::sin(angle);
					const glm::vec2 local[4] = {
						{-halfX, -halfY}, {halfX, -halfY},
						{halfX, halfY}, {-halfX, halfY}};
					for (uint32_t i = 0; i < 4; ++i)
					{
						BatchVertex& vertex = vertices[quadIndex * 4 + i];
						vertex.Position = {
							quad.Position.x + c * local[i].x - sn * local[i].y,
							quad.Position.y + sn * local[i].x + c * local[i].y};
						vertex.TexCoord = uv[i];
						vertex.Color = quad.Color;
					}

					batches.back().QuadCount++;
					++quadIndex;
				}

                spriteTimes.build = elapsed(buildStart);
				m_RenderStats.BatchCount = batches.size();

				// ---- Acquire space in this frame's vertex buffer ----

				const uint32_t frameIndex = m_FrameIndex;

				ANV_ASSERT(
					frameIndex < m_FrameQuadVBs.size(),
					"Invalid frame vertex buffer index!");

				auto vkVB = m_FrameQuadVBs[frameIndex].As<VulkanBuffer>();

				ANV_ASSERT(
					vkVB,
					"Frame vertex buffer is NULL!");

				auto vkIB = m_QuadIB.As<VulkanBuffer>();

				ANV_ASSERT(
					vkIB,
					"Quad index buffer is NULL!");

				const VkDeviceSize uploadSize =
					vertices.size() * sizeof(BatchVertex);

				VkDeviceSize &cursor = m_FrameVertexOffsets[frameIndex];

				ANV_ASSERT(
					cursor <= vkVB->GetSize() &&
						uploadSize <= vkVB->GetSize() - cursor,
					"Sprite batch vertex buffer capacity exceeded");

				const VkDeviceSize uploadOffset = cursor;

                const auto uploadStart = SpriteClock::now();
				if (uploadSize > 0)
				{
					vkVB->SetData(
						vertices.data(),
						uploadSize,
						uploadOffset);

					cursor += uploadSize;
				}

                spriteTimes.upload = elapsed(uploadStart);
                const auto commandStart = SpriteClock::now();
				const int32_t baseVertex =
					static_cast<int32_t>(
						uploadOffset / sizeof(BatchVertex));

				// ---- Record Vulkan commands ----

				auto vkCmd = cmd.As<VulkanCommandBuffer>();

				_renderTarget->Begin(cmd);
				pipeline->Bind(cmd);

				VkViewport vp{};
				vp.x = 0.0f;
				vp.y = 0.0f;
				vp.width = static_cast<float>(_renderTarget->GetWidth());
				vp.height = static_cast<float>(_renderTarget->GetHeight());
				vp.minDepth = 0.0f;
				vp.maxDepth = 1.0f;

				vkCmdSetViewport(vkCmd->Get(), 0, 1, &vp);

				VkRect2D sc{};
				sc.offset = {0, 0};
				sc.extent = {
					_renderTarget->GetWidth(),
					_renderTarget->GetHeight()};

				vkCmdSetScissor(vkCmd->Get(), 0, 1, &sc);

				VkBuffer vertexBuffer = vkVB->GetBuffer();
				VkDeviceSize vertexOffset = 0;

				vkCmdBindVertexBuffers(
					vkCmd->Get(),
					0,
					1,
					&vertexBuffer,
					&vertexOffset);

				vkCmdBindIndexBuffer(
					vkCmd->Get(),
					vkIB->GetBuffer(),
					0,
					VK_INDEX_TYPE_UINT32);

				auto vkPipeline = pipeline.As<VulkanPipeline>();

				VkPipelineLayout layout =
					vkPipeline->GetPipelineLayout();

				vkCmdPushConstants(
					vkCmd->Get(),
					layout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0,
					sizeof(glm::mat4),
					&sceneViewProjection);

				uint32_t drawCalls = 0;

				// ---- One draw call per contiguous texture batch ----

				for (const auto &batch : batches)
				{
					VkDescriptorSet textureSet =
						batch.Texture->GetDescriptorSet();

					vkCmdBindDescriptorSets(
						vkCmd->Get(),
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						layout,
						1,
						1,
						&textureSet,
						0,
						nullptr);

					vkCmdDrawIndexed(
						vkCmd->Get(),
						batch.QuadCount * 6,
						1,
						batch.FirstQuad * 6,
						baseVertex,
						0);

					++drawCalls;
				}

				m_RenderStats.DrawCalls = drawCalls;

				_renderTarget->End(cmd);
                spriteTimes.commands = elapsed(commandStart);
                m_RenderCmdChain->AccumulateSpriteTaskTimings(spriteTimes);
			});
	}

	void VulkanRenderAPI::DrawQuad(const glm::vec2 &position, float rotation, const glm::vec2 &size,
								   glm::vec4 color, Ref<Texture> texture, int layer)
	{
		m_QuadQueue.push_back({position, rotation, size, color, texture, layer});
		m_RenderStats.QuadCount++;
	}

	void VulkanRenderAPI::EndScene()
	{
		m_ImGuiLayer->EndFrame();
	}

	void VulkanRenderAPI::SetMainCamera(_shared<Camera2D> camera)
	{
		// Kept for RenderAPI compatibility. Vulkan rendering is explicitly
		// camera-per-DrawScene and does not use renderer-global camera state.
		(void)camera;
	}

	RendererStats VulkanRenderAPI::GetStats() { return m_RenderStats; }

	void VulkanRenderAPI::load_shader_lib()
	{
		auto spritepath = App::GetInstance()->GetFS().GetKeyVal("ShaderLib") / "sprite.glsl";
		m_SpriteShader = Shader::CreateInternal(spritepath.string(), m_Context);
	}

	void VulkanRenderAPI::build_2D_pipelines()
	{
		m_PipelineLibrary.Register("Sprite", [this](Ref<RenderPass> renderPass)
								   { return build_sprite_pipeline(renderPass); });
	}

	void VulkanRenderAPI::create_frames()
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		VkDevice device = vkCtx->GetDevice();
		m_Frames.resize(m_CreateInfo.swapchainImageCount);

		m_FrameQuadVBs.resize(m_CreateInfo.swapchainImageCount);

		m_FrameVertexOffsets.resize(
			m_CreateInfo.swapchainImageCount,
			0);

		VkSemaphoreCreateInfo semInfo{};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < m_CreateInfo.swapchainImageCount; i++)
		{
			ANV_VK_CHECK_RESULT(vkCreateSemaphore(device, &semInfo, nullptr, &m_Frames[i].sync.imageAvailable), "Failed to create imageAvailable semaphore!");
			ANV_VK_CHECK_RESULT(vkCreateSemaphore(device, &semInfo, nullptr, &m_Frames[i].sync.renderFinished), "Failed to create renderFinished semaphore!");
			ANV_VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &m_Frames[i].sync.inFlightFence), "Failed to create inFlight fence!");
			m_Frames[i].cmd = Ref<VulkanCommandBuffer>::Create(m_Context);

			// Allocate this frames VB
			BufferCreateInfo vbInfo{};
			vbInfo.Usage = BufferUsage::Vertex;
			vbInfo.Size =
				MaxBatchQuads * 4 * sizeof(BatchVertex);
			vbInfo.Dynamic = true;

			m_FrameQuadVBs[i] =
				Buffer::Create(m_Context, vbInfo);

			ANV_ASSERT(
				m_FrameQuadVBs[i],
				"Failed to create frame vertex buffer!");
		}
		m_FrameIndex = 0;
	}

	void VulkanRenderAPI::destroy_frames()
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		VkDevice device = vkCtx->GetDevice();
		for (auto &fr : m_Frames)
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
			fr.cmd = nullptr;
		}
		m_Frames.clear();
		m_FrameQuadVBs.clear();
		m_FrameVertexOffsets.clear();
	}

	void VulkanRenderAPI::recreate_swap()
	{
		m_RecreatingSwapchain.store(true);
		m_RenderCmdChain->Flush();
		auto vkCtx = m_Context->GetAs<VulkanContext>();
		vkCtx->IdleDevice();

		m_SwapchainTarget->ReleaseFramebuffers();
		m_Context->GetSwapchain()->ResetSwap();

		auto extent = vkCtx->GetSwapchain()->GetExtent();
		m_SwapchainTarget->Resize(extent.width, extent.height);
		m_PipelineLibrary.Clear();

		m_RecreatingSwapchain.store(false);
	}

	void VulkanRenderAPI::create_quad_buffers()
	{
		std::vector<uint32_t> indices(MaxBatchQuads * 6);

		for (uint32_t i = 0; i < MaxBatchQuads; ++i)
		{
			uint32_t baseVertex = i * 4;
			uint32_t baseIndex = i * 6;

			indices[baseIndex + 0] = baseVertex;
			indices[baseIndex + 1] = baseVertex + 1;
			indices[baseIndex + 2] = baseVertex + 2;
			indices[baseIndex + 3] = baseVertex + 2;
			indices[baseIndex + 4] = baseVertex + 3;
			indices[baseIndex + 5] = baseVertex;
		}

		BufferCreateInfo info{};
		info.Usage = BufferUsage::Index;
		info.Size = indices.size() * sizeof(uint32_t);
		info.InitialData = indices.data();

		m_QuadIB = Buffer::Create(m_Context, info);
	}

	void VulkanRenderAPI::create_descriptor_set_layout()
	{
		VkDevice device = m_Context->GetAs<VulkanContext>()->GetDevice();

		// Reserve set 0 without storing camera data in renderer-global state.
		// This preserves the existing texture ABI at set 1.
		VkDescriptorSetLayoutCreateInfo reservedLayoutInfo{};
		reservedLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		reservedLayoutInfo.bindingCount = 0;
		reservedLayoutInfo.pBindings = nullptr;

		ANV_VK_CHECK_RESULT(
			vkCreateDescriptorSetLayout(device, &reservedLayoutInfo, nullptr, &m_ReservedDescriptorSetLayout),
			"Failed to create reserved descriptor set layout!");

		VkDescriptorSetLayoutBinding textureBinding{};
		textureBinding.binding = 0;
		textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureBinding.descriptorCount = 1;
		textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
		textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		textureLayoutInfo.bindingCount = 1;
		textureLayoutInfo.pBindings = &textureBinding;

		ANV_VK_CHECK_RESULT(
			vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &m_TextureDescriptorSetLayout),
			"Failed to create texture descriptor set layout!");
	}

	void VulkanRenderAPI::create_white_texture()
	{
		const unsigned char white[4] = {255, 255, 255, 255};
		m_WhiteTexture = Ref<VulkanTexture>::Create(white, 1, 1, "Vulkan White Texture");
		ANV_ASSERT(m_WhiteTexture && m_WhiteTexture->IsGPUReady(), "Failed to create Vulkan white texture");
	}

	void VulkanRenderAPI::destroy_descriptor_resources()
	{
		VkDevice device = m_Context->GetAs<VulkanContext>()->GetDevice();

		if (m_TextureDescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(device, m_TextureDescriptorSetLayout, nullptr);
		if (m_ReservedDescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(device, m_ReservedDescriptorSetLayout, nullptr);

		m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
		m_ReservedDescriptorSetLayout = VK_NULL_HANDLE;
	}

	void VulkanRenderAPI::begin_batch() {}
	void VulkanRenderAPI::end_batch() {}

	Ref<GraphicsPipeline> VulkanRenderAPI::build_sprite_pipeline(Ref<RenderPass> renderPass)
	{
		VertexInputLayout quadLayout{};

		quadLayout.binding = 0;
		quadLayout.stride = sizeof(BatchVertex);

		quadLayout.AddAttribute(
			"Position",
			0,
			offsetof(BatchVertex, Position),
			sizeof(glm::vec2),
			sizeof(BatchVertex));

		quadLayout.AddAttribute(
			"TexCoord",
			1,
			offsetof(BatchVertex, TexCoord),
			sizeof(glm::vec2),
			sizeof(BatchVertex));

		quadLayout.AddAttribute(
			"Color",
			2,
			offsetof(BatchVertex, Color),
			sizeof(glm::vec4),
			sizeof(BatchVertex));

		auto pipeline = GraphicsPipeline::CreateInternal(m_Context, "Sprite Pipeline");
		pipeline->SetShaderStages(m_SpriteShader);
		pipeline->SetVertexInputLayout(&quadLayout);
		pipeline->SetRasterizationSettings(nullptr);
		pipeline.As<VulkanPipeline>()->SetDescriptorSetLayouts({m_ReservedDescriptorSetLayout,
																m_TextureDescriptorSetLayout});
		pipeline.As<VulkanPipeline>()->SetPushConstantRange(
			VK_SHADER_STAGE_VERTEX_BIT,
			sizeof(glm::mat4));
		pipeline->SetColorBlendSettings(nullptr);
		pipeline->SetRenderPass(renderPass);
		pipeline->Build();
		return pipeline;
	}
}

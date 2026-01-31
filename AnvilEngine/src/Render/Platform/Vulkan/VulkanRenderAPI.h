#pragma once
#include "Render/RenderAPI.h"
#include "Render/Renderer.h"
#include "Render/QueueChain.h"
#include "VulkanFrameResources.h"

namespace anv
{
	class VulkanRenderAPI
		: public RenderAPI
	{
	public:
		VulkanRenderAPI(Render2DCreateInfo _info);
		~VulkanRenderAPI() override {};

		virtual void DrawFrame() override;
		virtual void OnShutdown() override;

	private:
		void create_render_passes();
		void load_shader_lib();
		void build_pipeline();
		void create_frame_buffers();
		void create_frames();
		void destroy_frames();
		void recreate_swap();
	private:
		Render2DCreateInfo m_CreateInfo;
		Ref<RenderPass> m_RenderPass = nullptr;
		Ref<GraphicsPipeline>     m_Pipeline = nullptr;
		Ref<Shader> m_Shader = nullptr;
		_vec<Ref<Framebuffer>>    m_FrameBuffers{};
		_shared<QueueChain> m_RenderCmdChain = nullptr;

		// sync //
		
		std::vector<VulkanFrameResources> m_Frames;
		uint32_t m_FrameIndex = 0;

		std::atomic<bool> m_RecreatingSwapchain{ false };
		bool m_SwapRecreateFlag = false;
	};
}


#pragma once
#include "Render/RenderAPI.h"
#include "Render/Renderer.h"
#include "Render/QueueChain.h"
#include "VulkanFrameResources.h"
#include "Render/Buffer.h"

#include <Scene/SceneData.h>

namespace anv
{
	//struct PushContantData
	//{
	//	glm::mat4 model;
	//	glm::vec4 color;
	//};

	struct QuadSubmission
	{
		glm::vec2 Position;
		glm::vec2 Size;
		glm::vec4 Color;
	};

	class VulkanRenderAPI
		: public RenderAPI
	{
	public:
		VulkanRenderAPI(Render2DCreateInfo _info);
		~VulkanRenderAPI() override {};

		virtual void DrawFrame() override;
		virtual void OnShutdown() override;

		virtual void BeginScene() override;
		virtual void DrawQuad(const glm::vec2& position, const glm::vec2& size, glm::vec4 color) override;
		virtual void EndScene() override;

		virtual void SetMainCamera(_shared<Camera2D> camera) override;

	private:
		void create_render_passes();
		void load_shader_lib();
		void build_2D_pipelines();
		void create_frame_buffers();
		void create_frames();
		void destroy_frames();
		void recreate_swap();

		void create_quad_buffers();
		void create_descriptor_set_layout();
		void create_descriptor_pool();
		void create_camera_descriptor_set();

		void create_imgui_descriptor_pool();
		void init_imgui();
		void shutdown_imgui();
		void begin_imgui();
		void end_imgui(Ref<CommandBuffer> cmd);

	private:
		Render2DCreateInfo            m_CreateInfo         {};
		_vec<Ref<Framebuffer>>   m_FrameBuffers    {};
		Ref<RenderPass>                m_RenderPass       = nullptr;
		Ref<GraphicsPipeline>        m_Pipeline            = nullptr;
		Ref<GraphicsPipeline>        m_SpritePipeline    = nullptr;
		Ref<Shader>                       m_SpriteShader     = nullptr;
		_shared<QueueChain>        m_RenderCmdChain  = nullptr;
		_shared<AssetManager>     m_AssetManager = nullptr;
		_vec<QuadSubmission>      m_QuadQueue;

		// tmp
		Ref<Buffer> m_QuadVB;
		Ref<Buffer> m_QuadIB;
		Ref<Buffer> m_CameraUBO;

		VkDescriptorSetLayout m_CameraDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_CameraDescriptorSet = VK_NULL_HANDLE;

		VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;

		// sync //
		std::vector<VulkanFrameResources> m_Frames;
		uint32_t                                             m_FrameIndex = 0;

		// Window Resize //
		std::atomic<bool>                             m_RecreatingSwapchain{ false };
		bool m_SwapRecreateFlag                   = false;
	};
}


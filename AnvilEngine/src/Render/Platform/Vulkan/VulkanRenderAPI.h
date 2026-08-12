#pragma once
#include "Render/RenderAPI.h"
#include "Render/Renderer.h"
#include "Render/QueueChain.h"
#include "Render/PipelineLibrary.h"
#include "Render/Buffer.h"
#include "VulkanFrameResources.h"
#include "VulkanSwapChainTarget.h"

#include <Scene/SceneData.h>
#include <Render/RenderData.h>

namespace anv
{
	class AssetManager;

	//struct PushContantData
	//{
	//	glm::mat4 model;
	//	glm::vec4 color;
	//};

	class VulkanRenderAPI
		: public RenderAPI
	{
	public:
		VulkanRenderAPI(Render2DCreateInfo _info);
		~VulkanRenderAPI() override {};

		virtual void DrawFrame() override;
		virtual void OnShutdown() override;

		virtual void BeginScene(Ref<RenderTarget> _renderTarget) override;
		virtual void BeginScene() override;
		virtual void DrawScene(Ref<RenderTarget> _renderTarget, Ref<Camera2D> camera);
		virtual void DrawQuad(
			const glm::vec2& position,
			float rotation,
			const glm::vec2& size,
			glm::vec4 color,
			Ref<Texture> texture,
			int layer) override;
		virtual void EndScene() override;

		virtual void SetMainCamera(_shared<Camera2D> camera) override;

		virtual RendererStats GetStats() override;

	private:
		void load_shader_lib();
		void build_2D_pipelines();
		void create_frames();
		void destroy_frames();
		void recreate_swap();

		void create_quad_buffers();
		void create_descriptor_set_layout();
		void create_descriptor_pool();
		void create_camera_descriptor_set();

		void begin_batch();
		void end_batch();

		// ImGui //
		void create_imgui_descriptor_pool();
		void init_imgui();
		void shutdown_imgui();
		void begin_imgui();
		void end_imgui(Ref<CommandBuffer> cmd);

		Ref<GraphicsPipeline> build_sprite_pipeline(Ref<RenderPass> renderPass);

	private:
		Render2DCreateInfo            m_CreateInfo          {};
		_vec<QuadSubmission>      m_QuadQueue       {};
		Ref<GraphicsPipeline>        m_Pipeline                 = nullptr;

		PipelineLibrary m_PipelineLibrary;

		Ref<Shader>                       m_SpriteShader          = nullptr;
		_shared<QueueChain>        m_RenderCmdChain  = nullptr;
		_shared<AssetManager>     m_AssetManager       = nullptr;

		// tmp
		Ref<Buffer> m_QuadVB;
		Ref<Buffer> m_QuadIB;
		Ref<Buffer> m_CameraUBO;

		Ref<VulkanSwapchainRenderTarget> m_SwapchainTarget = nullptr;

		VkDescriptorSetLayout m_CameraDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool                             = VK_NULL_HANDLE;
		VkDescriptorSet m_CameraDescriptorSet                     = VK_NULL_HANDLE;

		VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;

		// sync //
		std::vector<VulkanFrameResources> m_Frames;
		uint32_t                                             m_FrameIndex = 0;

		// Window Resize //
		std::atomic<bool> m_RecreatingSwapchain{ false };
		bool m_SwapRecreateFlag = false;

		RendererStats m_RenderStats;
	};
}

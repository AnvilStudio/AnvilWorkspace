#pragma once
#include "Render/GraphicsPipeline.h"
#include "VulkanContext.h"
#include <vulkan/vulkan.h>

namespace anv
{

	struct VulkanPipelineCreateInfos
	{
		_vec<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo            dynamicState;
		VkPipelineViewportStateCreateInfo            viewportState;
		VkPipelineVertexInputStateCreateInfo        vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo   inputAssembly;
		VkPipelineRasterizationStateCreateInfo     rasterizer;
		VkPipelineMultisampleStateCreateInfo      multisampling;
		VkPipelineColorBlendAttachmentState      colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo        colorBlending;
		VkPipelineLayoutCreateInfo                      pipelineLayoutInfo;
		VkViewport                                              viewport;
		VkRect2D                                                 scissor;
		_vec<VkPipelineShaderStageCreateInfo>      stages;
		VkGraphicsPipelineCreateInfo                       pipelineCreateInfo;

		VkGraphicsPipelineCreateInfo* BuildInfo()
		{
			pipelineCreateInfo.pDynamicState          = &dynamicState;
			pipelineCreateInfo.pViewportState          = &viewportState;
			pipelineCreateInfo.pVertexInputState      = &vertexInputInfo;
			pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			pipelineCreateInfo.pRasterizationState    = &rasterizer;
			pipelineCreateInfo.pMultisampleState     = &multisampling;
			pipelineCreateInfo.pColorBlendState       = &colorBlending;
			pipelineCreateInfo.stageCount               = stages.size();
			pipelineCreateInfo.pStages                     = stages.data();

			return &pipelineCreateInfo;
		}

	};

	class VulkanPipeline
		: public GraphicsPipeline
	{
	public:
		VulkanPipeline(_shared<Context> _ctx, std::string& _dName);
		virtual ~VulkanPipeline() override;

		VkPipelineLayout GetPipelineLayout();
		VkPipeline GetPipeline() { return m_Pipeline; }

		void SetShaderStages(const Ref<Shader> _shader)                          override;
		void SetVertexInputLayout(const VertexInputLayout* _layout = nullptr)          override;
		void SetRasterizationSettings(const RasterizationSettings* _raster)  override;
		void SetColorBlendSettings(const ColorBlendSettings* _colbld={})        override;
		void SetRenderPass(const Ref<RenderPass> _rps)                           override;
		void SetDescriptorSetLayouts(const _vec<VkDescriptorSetLayout>& layouts={});
		void SetPushConstantRange(VkShaderStageFlags _stage=NULL, uint32_t _size=0);
		void Build()                                                                                      override;
		void Bind(_shared<QueueChain> _cmdq)                                        override;

		virtual void OnSave(Serializer& _ser) override;

		static VkFormat AttributeFormat(uint32_t size)
		{
			switch (size)
			{
			case sizeof(float) * 2: return VK_FORMAT_R32G32_SFLOAT;
			case sizeof(float) * 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case sizeof(float) * 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			default:
				ANV_LOG_FATAL("Unsupported vertex attribute size");
				return VK_FORMAT_UNDEFINED;
			}
		}
	private:
		void create_layout();

	private:
		VulkanPipelineCreateInfos m_CreateInfo{};
		VkPushConstantRange      m_PushRange{};
		VkPipeline                         m_Pipeline;
		VkPipelineLayout               m_PipelineLayout;
		VkRenderPass                    m_RenderPass;
		VulkanContext*                 m_VkContext;
		Ref<Shader>                     m_Shader;         
		std::vector<std::string>    m_EntryNames; 

		VkVertexInputBindingDescription m_VertexBindingDescription{};
		_vec<VkVertexInputAttributeDescription> m_VertexAttributeDescriptions{};
		_vec<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}

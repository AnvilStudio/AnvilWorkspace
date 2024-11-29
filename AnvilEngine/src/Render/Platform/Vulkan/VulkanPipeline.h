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

		VkPipelineDynamicStateCreateInfo       dynamicState;
		VkPipelineViewportStateCreateInfo      viewportState;
		VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineMultisampleStateCreateInfo   multisampling;
		VkPipelineColorBlendAttachmentState    colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo    colorBlending;
		VkPipelineLayoutCreateInfo             pipelineLayoutInfo;
		VkViewport viewport;
		VkRect2D scissor;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	};

	class VulkanPipeline
		: public GraphicsPipeline
	{
	public:
		VulkanPipeline(_shared<Context> _ctx);
		~VulkanPipeline();

		void SetShaderStages(Ref<Shader> _shader)                     override;
		void SetVertexInputLayout(VertexInputLayout* _layout)         override;
		void SetRasterizationSettings(RasterizationSettings* _raster) override;
		void SetColorBlendSettings(ColorBlendSettings* _colbld)       override;
		void Build() override;

	private:
		VulkanPipelineCreateInfos m_CreateInfo{};
		VkPipeline                m_Pipeline;
		VkPipelineLayout          m_PipelineLayout;
		VulkanContext*            m_VkContext;
	};
}

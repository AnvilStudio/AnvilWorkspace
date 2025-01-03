#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanRenderPass.h"

namespace anv
{
	VulkanPipeline::VulkanPipeline(_shared<Context> _ctx)
	{
		ANV_PROFILE_SCOPE()
		// Get the native Vk Context;
		m_VkContext = _ctx->GetNativeContextAs<VulkanContext>();

		// Set Viewport

		VkExtent2D scExtent = { 
			m_VkContext->GetSwapchain().GetExtent().width,
			m_VkContext->GetSwapchain().GetExtent().height 
		};

		m_CreateInfo.viewport = {};
		m_CreateInfo.viewport.x = 0.0f;
		m_CreateInfo.viewport.y = 0.0f;
		m_CreateInfo.viewport.width =  scExtent.width;
		m_CreateInfo.viewport.height = scExtent.height;
		m_CreateInfo.viewport.minDepth = 0.0f;
		m_CreateInfo.viewport.maxDepth = 1.0f;

		// Set scissor
		m_CreateInfo.scissor = {};
		m_CreateInfo.scissor.offset = { 0, 0 };
		m_CreateInfo.scissor.extent = scExtent;

		// Set Dynamic states
		m_CreateInfo.dynamicState = {};
		m_CreateInfo.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		m_CreateInfo.dynamicState.dynamicStateCount = static_cast<uint32_t>(m_CreateInfo.dynamicStates.size());
		m_CreateInfo.dynamicState.pDynamicStates = m_CreateInfo.dynamicStates.data();

		m_CreateInfo.viewportState = {};
		m_CreateInfo.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_CreateInfo.viewportState.viewportCount = 1;
		m_CreateInfo.viewportState.pViewports = &m_CreateInfo.viewport;
		m_CreateInfo.viewportState.scissorCount  = 1;
		m_CreateInfo.viewportState.pScissors = &m_CreateInfo.scissor;

		// Set Input assembly info
		m_CreateInfo.inputAssembly = {};
		m_CreateInfo.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_CreateInfo.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		m_CreateInfo.inputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	VulkanPipeline::~VulkanPipeline()
	{
		//vkDestroyPipelineLayout(m_VkContext->GetDevice(), m_PipelineLayout, nullptr);
	}

	void VulkanPipeline::SetShaderStages(Ref<Shader> _shader)
	{
		auto vkshaders = _shader->GetAs<VulkanShader>()->GetShaderStages();
		m_CreateInfo.stages.resize(vkshaders.size());
		m_CreateInfo.stages[0] = vkshaders[0];
		m_CreateInfo.stages[1] = vkshaders[1];
	}

	void VulkanPipeline::SetVertexInputLayout(VertexInputLayout* _layout)
	{
		m_CreateInfo.vertexInputInfo = {};
		m_CreateInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_CreateInfo.vertexInputInfo.vertexBindingDescriptionCount = 0;
		m_CreateInfo.vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		m_CreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = 0;
		m_CreateInfo.vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

		ANV_LOG_DEBUG("Set Pipeline Vertex Input");
	}

	void VulkanPipeline::SetRasterizationSettings(RasterizationSettings* _raster)
	{
		m_CreateInfo.rasterizer = {};
		m_CreateInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_CreateInfo.rasterizer.depthClampEnable = VK_FALSE;
		m_CreateInfo.rasterizer.rasterizerDiscardEnable = VK_FALSE;
		m_CreateInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		m_CreateInfo.rasterizer.lineWidth = 1.0f;
		m_CreateInfo.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		m_CreateInfo.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		m_CreateInfo.rasterizer.depthBiasEnable = VK_FALSE;
		m_CreateInfo.rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		m_CreateInfo.rasterizer.depthBiasClamp = 0.0f; // Optional
		m_CreateInfo.rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// TODO: needs its own settup fn
		m_CreateInfo.multisampling = {};
		m_CreateInfo.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_CreateInfo.multisampling.sampleShadingEnable = VK_FALSE;
		m_CreateInfo.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		m_CreateInfo.multisampling.minSampleShading = 1.0f; // Optional
		m_CreateInfo.multisampling.pSampleMask = nullptr; // Optional
		m_CreateInfo.multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		m_CreateInfo.multisampling.alphaToOneEnable = VK_FALSE; // Optional

		ANV_LOG_DEBUG("Set Pipeline rasterization");
	}

	void VulkanPipeline::SetColorBlendSettings(ColorBlendSettings* _colbld)
	{
		m_CreateInfo.colorBlendAttachment = {};
		m_CreateInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_CreateInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		m_CreateInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		m_CreateInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		m_CreateInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		m_CreateInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		m_CreateInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		m_CreateInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
		m_CreateInfo.colorBlendAttachment.blendEnable = VK_TRUE;
		m_CreateInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_CreateInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		m_CreateInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		m_CreateInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_CreateInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_CreateInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		m_CreateInfo.colorBlending = {};
		m_CreateInfo.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_CreateInfo.colorBlending.logicOpEnable = VK_FALSE;
		m_CreateInfo.colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		m_CreateInfo.colorBlending.attachmentCount = 1;
		m_CreateInfo.colorBlending.pAttachments = &m_CreateInfo.colorBlendAttachment;
		m_CreateInfo.colorBlending.blendConstants[0] = 0.0f; // Optional
		m_CreateInfo.colorBlending.blendConstants[1] = 0.0f; // Optional
		m_CreateInfo.colorBlending.blendConstants[2] = 0.0f; // Optional
		m_CreateInfo.colorBlending.blendConstants[3] = 0.0f; // Optional

		// TODO: Needs settup fn
		m_CreateInfo.pipelineLayoutInfo = {};
		m_CreateInfo.pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		m_CreateInfo.pipelineLayoutInfo.setLayoutCount = 0; // Optional
		m_CreateInfo.pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		m_CreateInfo.pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		m_CreateInfo.pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		ANV_VK_CHECK_RESULT(vkCreatePipelineLayout(m_VkContext->GetDevice(), &m_CreateInfo.pipelineLayoutInfo, 
			nullptr, &m_PipelineLayout), "Failed to create pipeline layout!")

		ANV_LOG_DEBUG("Set Pipeline Color Blend");
	}

    void VulkanPipeline::SetRenderPass(RenderPass* _rps)
    {
		m_RenderPass = _rps->GetAs<VulkanRenderPass>()->GetRaw();
    }

	void VulkanPipeline::Build()
	{
		VkGraphicsPipelineCreateInfo* info = m_CreateInfo.BuildInfo();
		info->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info->layout = m_PipelineLayout;
		info->renderPass = m_RenderPass;
		info->pDepthStencilState = nullptr;
		info->subpass = 0;
		info->basePipelineHandle = VK_NULL_HANDLE;
		info->basePipelineIndex = -1;

		ANV_VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_VkContext->GetDevice(), VK_NULL_HANDLE, 1, info, nullptr, &m_Pipeline),
			"Failed to create Vk graphics pipeline")
	}

	void VulkanPipeline::Destroy()
	{
		vkDestroyPipeline(m_VkContext->GetDevice(), m_Pipeline, nullptr);
		vkDestroyPipelineLayout(m_VkContext->GetDevice(), m_PipelineLayout, nullptr);
	}
}
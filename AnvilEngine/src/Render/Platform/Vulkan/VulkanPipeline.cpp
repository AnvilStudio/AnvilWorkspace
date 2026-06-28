#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"

namespace anv
{
	VulkanPipeline::VulkanPipeline(_shared<Context> _ctx, std::string& _dName)
		: GraphicsPipeline(_ctx, _dName),
		m_Pipeline(VK_NULL_HANDLE),
  		m_PipelineLayout(VK_NULL_HANDLE),
  		m_RenderPass(VK_NULL_HANDLE)
	{
		ANV_PROFILE_SCOPE()
		// Get the native Vk Context;
		m_VkContext = _ctx->GetAs<VulkanContext>();


		// Zero all create infos up front
		m_CreateInfo.dynamicState      = {};
		m_CreateInfo.viewportState     = {};
		m_CreateInfo.vertexInputInfo   = {};
		m_CreateInfo.inputAssembly     = {};
		m_CreateInfo.rasterizer        = {};
		m_CreateInfo.multisampling     = {};
		m_CreateInfo.colorBlendAttachment = {};
		m_CreateInfo.colorBlending     = {};
		m_CreateInfo.pipelineLayoutInfo= {};
		m_CreateInfo.pipelineCreateInfo= {};

		// Set Viewport

		VkExtent2D scExtent = { 
			m_VkContext->GetSwapchain()->GetExtent().width,
			m_VkContext->GetSwapchain()->GetExtent().height 
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
		ANV_PROFILE_SCOPE();
    	if (m_Pipeline)       vkDestroyPipeline(m_VkContext->GetDevice(), m_Pipeline, nullptr);
    	if (m_PipelineLayout) vkDestroyPipelineLayout(m_VkContext->GetDevice(), m_PipelineLayout, nullptr);

	}

	VkPipelineLayout VulkanPipeline::GetPipelineLayout()
	{
		return m_PipelineLayout;
	}

	void VulkanPipeline::SetShaderStages(const Ref<Shader> _shader)
	{
		m_Shader = _shader; // keep alive through Build()
		auto vkshaders = _shader.As<VulkanShader>()->GetShaderStages();
		m_CreateInfo.stages = vkshaders; // copy all stages safely

		// fix for pName lifetime issue:
		m_EntryNames.clear();
		m_EntryNames.reserve(m_CreateInfo.stages.size());
		for (auto& st : m_CreateInfo.stages) {
			if (st.pName) {
				m_EntryNames.emplace_back(st.pName);
				st.pName = m_EntryNames.back().c_str(); // point to our owned copy
			}
		}
	}

	void VulkanPipeline::SetVertexInputLayout(const VertexInputLayout* _layout)
	{
		if (_layout == nullptr)
		{
			m_CreateInfo.vertexInputInfo = {};
			m_CreateInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			m_CreateInfo.vertexInputInfo.vertexBindingDescriptionCount = 0;
			m_CreateInfo.vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
			m_CreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = 0;
			m_CreateInfo.vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
		}
		else
		{
			
			m_VertexBindingDescription.binding = _layout->binding;
			m_VertexBindingDescription.stride = _layout->stride;
			m_VertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			for (VertexAttribute att : _layout->attributes)
			{
				VkVertexInputAttributeDescription desc{};
				desc.binding = _layout->binding;
				desc.location = att.location;
				desc.format = AttributeFormat(att.size);
				desc.offset = att.offset;

				m_VertexAttributeDescriptions.push_back(desc);
			}

			m_CreateInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			m_CreateInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
			m_CreateInfo.vertexInputInfo.pVertexBindingDescriptions = &m_VertexBindingDescription;
			m_CreateInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexAttributeDescriptions.size());
			m_CreateInfo.vertexInputInfo.pVertexAttributeDescriptions = m_VertexAttributeDescriptions.data();
		}

		ANV_LOG_DEBUG("Set Pipeline Vertex Input");
	}

	void VulkanPipeline::SetRasterizationSettings(const RasterizationSettings* _raster)
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

	void VulkanPipeline::SetColorBlendSettings(const ColorBlendSettings* _colbld)
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


		

		ANV_LOG_DEBUG("Set Pipeline Color Blend");
	}

	void VulkanPipeline::SetRenderPass(const Ref<RenderPass> _rps)
    {
		m_RenderPass = _rps.As<VulkanRenderPass>()->Get();
    }

	void VulkanPipeline::SetDescriptorSetLayouts(const _vec<VkDescriptorSetLayout>& layouts)
	{
		m_DescriptorSetLayouts = layouts;
	}

	void VulkanPipeline::SetPushConstantRange(VkShaderStageFlags _stage, uint32_t _size)
	{
		m_PushRange.stageFlags = _stage;
		m_PushRange.offset = 0;
		m_PushRange.size = _size;


	}

	void VulkanPipeline::Build()
	{
		ANV_ASSERT(m_RenderPass != VK_NULL_HANDLE, "Render pass not set");
		create_layout();
		ANV_ASSERT(m_PipelineLayout != VK_NULL_HANDLE, "Pipeline layout not created");

		auto* info = m_CreateInfo.BuildInfo();
		*info = {}; // full reset before fill
		info->sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info->layout              = m_PipelineLayout;
		info->renderPass          = m_RenderPass;
		info->pVertexInputState   = &m_CreateInfo.vertexInputInfo;
		info->pInputAssemblyState = &m_CreateInfo.inputAssembly;
		info->pViewportState      = &m_CreateInfo.viewportState;
		info->pRasterizationState = &m_CreateInfo.rasterizer;
		info->pMultisampleState   = &m_CreateInfo.multisampling;
		info->pColorBlendState    = &m_CreateInfo.colorBlending;
		info->pDynamicState       = &m_CreateInfo.dynamicState;
		info->pDepthStencilState  = nullptr;
		info->subpass             = 0;
		info->basePipelineHandle  = VK_NULL_HANDLE;
		info->basePipelineIndex   = -1;
		info->stageCount          = static_cast<uint32_t>(m_CreateInfo.stages.size());
		info->pStages             = m_CreateInfo.stages.data();

		ANV_VK_CHECK_RESULT(
			vkCreateGraphicsPipelines(m_VkContext->GetDevice(), VK_NULL_HANDLE, 1, info, nullptr, &m_Pipeline),
			"Failed to create Vk graphics pipeline"
		);
	}

	void VulkanPipeline::Bind(Ref<CommandBuffer> _cmd)
	{
		vkCmdBindPipeline(_cmd.As<VulkanCommandBuffer>()->Get(),
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);		
	}

	void VulkanPipeline::OnSave(Serializer& _ser)
	{
		_ser.Object("Spec", [&]
			{
				_ser.Field("Type", "GraphicsPipeline");
				_ser.Field("API", "VK");
				_ser.Field("Name", m_Name);
				_ser.Object("PipelineInfo", [&] {
					

				});
			});
	}

	void VulkanPipeline::create_layout()
	{
		m_CreateInfo.pipelineLayoutInfo = {};
		m_CreateInfo.pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		// descriptor sets
		m_CreateInfo.pipelineLayoutInfo.setLayoutCount =
			static_cast<uint32_t>(m_DescriptorSetLayouts.size());

		m_CreateInfo.pipelineLayoutInfo.pSetLayouts =
			m_DescriptorSetLayouts.empty()
			? nullptr
			: m_DescriptorSetLayouts.data();

		// push constants
		if (m_PushRange.size > 0)
		{
			m_CreateInfo.pipelineLayoutInfo.pushConstantRangeCount = 1;
			m_CreateInfo.pipelineLayoutInfo.pPushConstantRanges = &m_PushRange;
		}
		else
		{
			m_CreateInfo.pipelineLayoutInfo.pushConstantRangeCount = 0;
			m_CreateInfo.pipelineLayoutInfo.pPushConstantRanges = nullptr;
		}

		ANV_VK_CHECK_RESULT(
			vkCreatePipelineLayout(
				m_VkContext->GetDevice(), 
				&m_CreateInfo.pipelineLayoutInfo,
			    nullptr, &m_PipelineLayout)
			, "Failed to create pipeline layout!"
		)
	}
}
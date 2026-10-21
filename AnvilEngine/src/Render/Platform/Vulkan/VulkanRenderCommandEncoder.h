#pragma once
#include "Render/RenderCommandEncoder.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
namespace anv {
class VulkanRenderCommandEncoder final : public RenderCommandEncoder {
public:
    explicit VulkanRenderCommandEncoder(Ref<CommandBuffer> cmd)
        : m_Cmd(cmd.As<VulkanCommandBuffer>()) {}
    void BindPipeline(Ref<GraphicsPipeline> pipeline) override {
        m_Pipeline = pipeline.As<VulkanPipeline>();
        ANV_ASSERT(m_Pipeline, "A Vulkan overlay requires a Vulkan pipeline");
        m_Pipeline->Bind(m_Cmd);
    }
    void PushConstants(ShaderStage stages, const void* data, uint32_t size, uint32_t offset = 0) override {
        ANV_ASSERT(m_Pipeline, "BindPipeline must precede PushConstants");
        ANV_ASSERT(data && size, "PushConstants requires data");
        uint32_t mask = static_cast<uint32_t>(stages);
        VkShaderStageFlags flags = 0;
        if (mask & 1u) flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (mask & 2u) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        ANV_ASSERT(flags, "PushConstants requires shader stage flags");
        // Caller must respect the pipeline's declared push-constant range.
        vkCmdPushConstants(m_Cmd->Get(), m_Pipeline->GetPipelineLayout(),
                           flags, offset, size, data);
    }
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1,
              uint32_t firstVertex = 0, uint32_t firstInstance = 0) override {
        ANV_ASSERT(m_Pipeline, "BindPipeline must precede Draw");
        vkCmdDraw(m_Cmd->Get(), vertexCount, instanceCount, firstVertex, firstInstance);
    }
private:
    Ref<VulkanCommandBuffer> m_Cmd;
    Ref<VulkanPipeline> m_Pipeline;
};
}

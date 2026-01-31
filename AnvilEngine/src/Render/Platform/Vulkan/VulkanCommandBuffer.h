#pragma once
#include "Render/CommandBuffer.h"
#include "VulkanContext.h"

namespace anv
{
    class VulkanCommandBuffer : public CommandBuffer
    {
    public:
        VulkanCommandBuffer(_shared<Context> _ctx);
        virtual void Begin()  override;
        virtual void End()    override;
        virtual void Submit() override {};
        void Submit(VkSemaphore _vkWaitSemaphore, VkSemaphore _vkSignalSemaphore, VkFence _vkFence);
        virtual void Reset()  override;
        
        VkCommandBuffer Get() { return m_CmdBuffer; }

    private:
        VkCommandBuffer m_CmdBuffer;
    };
} // namespace anv

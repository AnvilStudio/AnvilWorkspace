#pragma once

#include "Layer/ImGuiLayer.h"

#include <vulkan/vulkan.h>

namespace anv
{
    class VulkanImGuiLayer final : public ImGuiLayer
    {
    public:
        VulkanImGuiLayer(
            std::shared_ptr<Context> context,
            Ref<RenderTarget> swapchainTarget,
            std::string iniPath);
        ~VulkanImGuiLayer() override;

    private:
        bool InitializeBackend() override;
        void ShutdownBackend() override;
        void BeginBackendFrame(void* nativeRenderPass) override;
        void RenderDrawData(const ImGuiRenderInfo& renderInfo) override;
        bool SupportsPlatformViewports() const override;

        Ref<RenderTarget> m_SwapchainTarget;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        bool m_GlfwInitialized = false;
        bool m_VulkanInitialized = false;
    };
}

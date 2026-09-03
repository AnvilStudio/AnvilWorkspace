#include "VulkanImGuiLayer.h"

#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanRenderPass.h"
#include "Render/RenderTarget.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <iterator>
#include <utility>

namespace anv
{
    VulkanImGuiLayer::VulkanImGuiLayer(
        std::shared_ptr<Context> context,
        Ref<RenderTarget> swapchainTarget,
        std::string iniPath)
        : ImGuiLayer(std::move(context), std::move(iniPath)),
          m_SwapchainTarget(std::move(swapchainTarget))
    {
    }

    VulkanImGuiLayer::~VulkanImGuiLayer()
    {
        Shutdown();
    }

    bool VulkanImGuiLayer::InitializeBackend()
    {
        auto* vkContext = m_Context->GetAs<VulkanContext>();
        if (!vkContext || !m_SwapchainTarget)
            return false;

        const VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * static_cast<uint32_t>(std::size(poolSizes));
        poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
        poolInfo.pPoolSizes = poolSizes;
        if (vkCreateDescriptorPool(vkContext->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
            return false;

        m_GlfwInitialized = ImGui_ImplGlfw_InitForVulkan(vkContext->GetWinHandle(), true);
        if (!m_GlfwInitialized)
            return false;

        ImGui_ImplVulkan_InitInfo init{};
        init.ApiVersion = VK_API_VERSION_1_3;
        init.Instance = vkContext->GetInstance();
        init.PhysicalDevice = vkContext->GetPhysicalDevice();
        init.Device = vkContext->GetDevice();
        init.QueueFamily = vkContext->GetQueueFamilies().graphicsFamily.value();
        init.Queue = vkContext->GetGraphicsQueue();
        init.DescriptorPool = m_DescriptorPool;
        init.MinImageCount = vkContext->GetSwapchain()->GetImageCount();
        init.ImageCount = vkContext->GetSwapchain()->GetImageCount();
        init.PipelineCache = VK_NULL_HANDLE;
        init.PipelineInfoMain.RenderPass = m_SwapchainTarget->GetRenderPass().As<VulkanRenderPass>()->Get();
        init.PipelineInfoMain.Subpass = 0;
        init.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init.UseDynamicRendering = false;
        init.Allocator = nullptr;
        init.CheckVkResultFn = nullptr;
        init.MinAllocationSize = 1024 * 1024;
        m_VulkanInitialized = ImGui_ImplVulkan_Init(&init);
        return m_VulkanInitialized;
    }

    void VulkanImGuiLayer::ShutdownBackend()
    {
        auto* vkContext = m_Context->GetAs<VulkanContext>();
        vkContext->IdleDevice();

        if (m_VulkanInitialized)
            ImGui_ImplVulkan_Shutdown();
        if (m_GlfwInitialized)
            ImGui_ImplGlfw_Shutdown();
        if (m_DescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(vkContext->GetDevice(), m_DescriptorPool, nullptr);

        m_VulkanInitialized = false;
        m_GlfwInitialized = false;
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    void VulkanImGuiLayer::BeginBackendFrame(void*)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
    }

    void VulkanImGuiLayer::RenderDrawData(const ImGuiRenderInfo& renderInfo)
    {
        auto commandBuffer = renderInfo.commandBuffer.As<VulkanCommandBuffer>();
        if (commandBuffer)
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer->Get());
    }

    bool VulkanImGuiLayer::SupportsPlatformViewports() const
    {
#if defined(PLATFORM_APPLE_VK)
        return false;
#else
        return true;
#endif
    }
}

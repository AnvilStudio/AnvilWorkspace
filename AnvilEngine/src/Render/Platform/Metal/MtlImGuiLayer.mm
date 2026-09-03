#include "MtlImGuiLayer.h"

#include "MtlContext.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_metal.h>
#include <imgui.h>

#import <Metal/Metal.h>

#include <utility>

namespace anv
{
    MetalImGuiLayer::MetalImGuiLayer(std::shared_ptr<Context> context, std::string iniPath)
        : ImGuiLayer(std::move(context), std::move(iniPath))
    {
    }

    MetalImGuiLayer::~MetalImGuiLayer()
    {
        Shutdown();
    }

    bool MetalImGuiLayer::InitializeBackend()
    {
        auto* metalContext = m_Context->GetAs<MetalContext>();
        m_GlfwInitialized = ImGui_ImplGlfw_InitForOther(metalContext->GetWindowHandle(), true);
        if (!m_GlfwInitialized)
            return false;

        id<MTLDevice> device = (__bridge id<MTLDevice>)metalContext->GetDevice();
        m_MetalInitialized = ImGui_ImplMetal_Init(device);
        return m_MetalInitialized;
    }

    void MetalImGuiLayer::ShutdownBackend()
    {
        if (m_MetalInitialized)
            ImGui_ImplMetal_Shutdown();
        if (m_GlfwInitialized)
            ImGui_ImplGlfw_Shutdown();

        m_MetalInitialized = false;
        m_GlfwInitialized = false;
    }

    void MetalImGuiLayer::BeginBackendFrame(void* nativeRenderPass)
    {
        MTLRenderPassDescriptor* renderPass = (__bridge MTLRenderPassDescriptor*)nativeRenderPass;
        ImGui_ImplMetal_NewFrame(renderPass);
        ImGui_ImplGlfw_NewFrame();
    }

    void MetalImGuiLayer::RenderDrawData(const ImGuiRenderInfo& renderInfo)
    {
        id<MTLCommandBuffer> commandBuffer = (__bridge id<MTLCommandBuffer>)renderInfo.nativeCommandBuffer;
        id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)renderInfo.nativeRenderEncoder;
        if (commandBuffer && encoder)
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
    }
}

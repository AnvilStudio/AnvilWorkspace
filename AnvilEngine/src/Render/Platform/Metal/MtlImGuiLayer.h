#pragma once

#include "Layer/ImGuiLayer.h"

namespace anv
{
    class MetalImGuiLayer final : public ImGuiLayer
    {
    public:
        MetalImGuiLayer(std::shared_ptr<Context> context, std::string iniPath);
        ~MetalImGuiLayer() override;

    private:
        bool InitializeBackend() override;
        void ShutdownBackend() override;
        void BeginBackendFrame(void* nativeRenderPass) override;
        void RenderDrawData(const ImGuiRenderInfo& renderInfo) override;

        bool m_GlfwInitialized = false;
        bool m_MetalInitialized = false;
    };
}

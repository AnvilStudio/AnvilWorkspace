#pragma once

#include "Core/Reference.h"
#include "Render/CommandBuffer.h"

#include <memory>
#include <string>

namespace anv
{
    class Context;
    class RenderTarget;

    struct ImGuiRenderInfo
    {
        Ref<CommandBuffer> commandBuffer = nullptr;
        void* nativeCommandBuffer = nullptr;
        void* nativeRenderEncoder = nullptr;
    };

    /**
     * Owns the Dear ImGui context and the graphics/window backend lifecycle.
     *
     * Render APIs only tell this layer when a frame starts and provide the
     * native handles required to encode its draw data. They do not initialize
     * or directly call Dear ImGui backends themselves.
     */
    class ImGuiLayer
    {
    public:
        virtual ~ImGuiLayer() = default;

        static std::unique_ptr<ImGuiLayer> Create(
            const std::shared_ptr<Context>& context,
            const Ref<RenderTarget>& swapchainTarget,
            std::string iniPath);

        void BeginFrame(void* nativeRenderPass = nullptr);
        void EndFrame();
        void Render(const ImGuiRenderInfo& renderInfo);
        void FinishFrame();
        void Shutdown();

        const std::string& GetIniPath() const { return m_IniPath; }

    protected:
        ImGuiLayer(std::shared_ptr<Context> context, std::string iniPath);

        bool Initialize();

        virtual bool InitializeBackend() = 0;
        virtual void ShutdownBackend() = 0;
        virtual void BeginBackendFrame(void* nativeRenderPass) = 0;
        virtual void RenderDrawData(const ImGuiRenderInfo& renderInfo) = 0;
        virtual bool SupportsPlatformViewports() const { return false; }

        std::shared_ptr<Context> m_Context;

    private:
        std::string m_IniPath;
        bool m_Initialized = false;
        bool m_FrameActive = false;
    };
}

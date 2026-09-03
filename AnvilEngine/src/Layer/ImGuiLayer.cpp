#include "ImGuiLayer.h"

#include "Core/Macros.h"
#include "Render/RenderTarget.h"
#include "Util/UMacros.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <filesystem>
#include <utility>

#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
#include "Render/Platform/Vulkan/VulkanImGuiLayer.h"
#elif defined(PLATFORM_APPLE)
#include "Render/Platform/Metal/MtlImGuiLayer.h"
#endif

namespace anv
{
    ImGuiLayer::ImGuiLayer(std::shared_ptr<Context> context, std::string iniPath)
        : m_Context(std::move(context)), m_IniPath(std::move(iniPath))
    {
    }

    std::unique_ptr<ImGuiLayer> ImGuiLayer::Create(
        const std::shared_ptr<Context>& context,
        const Ref<RenderTarget>& swapchainTarget,
        std::string iniPath)
    {
        std::unique_ptr<ImGuiLayer> layer;

#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
        layer = std::make_unique<VulkanImGuiLayer>(context, swapchainTarget, std::move(iniPath));
#elif defined(PLATFORM_APPLE)
        (void)swapchainTarget;
        layer = std::make_unique<MetalImGuiLayer>(context, std::move(iniPath));
#else
#error "No ImGui backend is available for this platform"
#endif

        if (!layer->Initialize())
            return nullptr;

        return layer;
    }

    bool ImGuiLayer::Initialize()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (SupportsPlatformViewports())
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        if (!m_IniPath.empty())
        {
            const std::filesystem::path path(m_IniPath);
            if (path.has_parent_path())
            {
                std::error_code error;
                std::filesystem::create_directories(path.parent_path(), error);
                if (error)
                    ANV_LOG_WARN("Failed to create ImGui ini directory '%s': %s",
                        path.parent_path().string().c_str(), error.message().c_str())
            }

            // ImGui stores this pointer; m_IniPath owns the backing storage for
            // the entire lifetime of the context.
            io.IniFilename = m_IniPath.c_str();
        }
        else
        {
            io.IniFilename = nullptr;
        }

        ImGui::StyleColorsDark();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        if (!InitializeBackend())
        {
            ShutdownBackend();
            ImGui::DestroyContext();
            return false;
        }

        m_Initialized = true;
        ANV_LOG_INFO("Initialized ImGui (ini: %s)",
            m_IniPath.empty() ? "disabled" : m_IniPath.c_str())
        return true;
    }

    void ImGuiLayer::BeginFrame(void* nativeRenderPass)
    {
        if (!m_Initialized)
            return;

        BeginBackendFrame(nativeRenderPass);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        m_FrameActive = true;
    }

    void ImGuiLayer::EndFrame()
    {
        if (!m_Initialized || !m_FrameActive)
            return;

        ImGui::Render();
        m_FrameActive = false;
    }

    void ImGuiLayer::Render(const ImGuiRenderInfo& renderInfo)
    {
        if (m_Initialized)
            RenderDrawData(renderInfo);
    }

    void ImGuiLayer::FinishFrame()
    {
        if (!m_Initialized)
            return;

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImGuiLayer::Shutdown()
    {
        if (!m_Initialized)
            return;

        if (m_FrameActive)
            EndFrame();

        ShutdownBackend();
        ImGui::DestroyContext();
        m_Initialized = false;
        ANV_LOG_INFO("Shutdown ImGui");
    }
}

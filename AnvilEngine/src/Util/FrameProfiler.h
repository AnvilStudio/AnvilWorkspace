#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace anv
{
    // Single-threaded frame profiler. Scopes must complete before EndFrame().
    // Results are CPU wall-clock times; GPU duration requires GPU timestamps.
    class FrameProfiler
    {
    public:
        enum class Zone : std::uint8_t
        {
            Frame, Input, LayerUpdate, RendererBegin, AppUpdate,
            LayerRender, EditorUI, RendererEnd, Submit,
            Physics, BodyManagement, PhysicsToBox2D, PhysicsCapture, Box2D, Box2DToScene,
            Script, SceneRender, SpriteLookup, SpriteTransform, SpriteSubmit, PhysicsFixedSteps, GPUFenceWait, SwapchainAcquire, RenderSubmitWait, RenderQueueSwap, RenderWorkerWait, ImGuiFinish, Present, MetalEncode, MetalCommit, WorkerBegin, WorkerTasks, WorkerEnd, WorkerSubmit, WorkerTotal, SpriteSort, SpriteBuild, SpriteUpload, SpriteCommands, Count
        };
        static constexpr std::size_t Count = static_cast<std::size_t>(Zone::Count);
        static constexpr std::size_t History = 120;
        struct Result { float last = 0.0f, average = 0.0f, max = 0.0f; };

        static constexpr const char* Name(Zone zone)
        {
            switch (zone)
            {
                case Zone::Frame: return "Frame";
                case Zone::Input: return "Input & events";
                case Zone::LayerUpdate: return "Layer updates";
                case Zone::RendererBegin: return "Begin scene";
                case Zone::AppUpdate: return "App update";
                case Zone::LayerRender: return "Layer rendering";
                case Zone::EditorUI: return "Editor UI";
                case Zone::RendererEnd: return "End scene";
                case Zone::Submit: return "Draw/present";
                case Zone::Physics: return "Physics total";
                case Zone::BodyManagement: return "Physics body management";
                case Zone::PhysicsToBox2D: return "Scene to Box2D";
                case Zone::PhysicsCapture: return "Physics pre/post capture";
                case Zone::Box2D: return "Box2D simulation";
                case Zone::Box2DToScene: return "Box2D to scene";
                case Zone::Script: return "Python scripts";
                case Zone::SceneRender: return "Scene sprites";
                case Zone::SpriteLookup: return "Sprite asset lookup";
                case Zone::SpriteTransform: return "Sprite world transform";
                case Zone::SpriteSubmit: return "Sprite draw submission";
                case Zone::PhysicsFixedSteps: return "Physics fixed steps / frame";
                case Zone::GPUFenceWait: return "GPU fence wait";
                case Zone::SwapchainAcquire: return "Swapchain acquire";
                case Zone::RenderSubmitWait: return "Render submission total";
                case Zone::RenderQueueSwap: return "Render queue swap";
                case Zone::RenderWorkerWait: return "Render worker wait";
                case Zone::ImGuiFinish: return "ImGui finish frame";
                case Zone::Present: return "Swapchain present";
                case Zone::MetalEncode: return "Metal encode and UI";
                case Zone::MetalCommit: return "Metal commit/present enqueue";
                case Zone::WorkerBegin: return "Worker command begin";
                case Zone::WorkerTasks: return "Worker execute tasks";
                case Zone::WorkerEnd: return "Worker command end";
                case Zone::WorkerSubmit: return "Worker Vulkan submit";
                case Zone::WorkerTotal: return "Worker processing total";
                case Zone::SpriteSort: return "Worker sprite sort";
                case Zone::SpriteBuild: return "Worker sprite vertices/batches";
                case Zone::SpriteUpload: return "Worker sprite buffer upload";
                case Zone::SpriteCommands: return "Worker sprite command recording";
                default: return "Unknown";
            }
        }

        static void BeginFrame()
        {
            s_Current.fill(0.0f);
        }

        static void Record(Zone zone, float milliseconds)
        {
            s_Current[static_cast<std::size_t>(zone)] += milliseconds;
        }

        static void EndFrame()
        {
            s_Samples[s_Next] = s_Current;
            s_Next = (s_Next + 1) % History;
            if (s_Filled < History) ++s_Filled;
        }

        static Result Get(Zone zone)
        {
            const std::size_t index = static_cast<std::size_t>(zone);
            Result result;
            // Show the most recently completed frame, not the partial frame currently being drawn.
            result.last = s_Filled ? s_Samples[(s_Next + History - 1) % History][index] : 0.0f;
            if (!s_Filled) return result;
            float total = 0.0f;
            for (std::size_t i = 0; i < s_Filled; ++i)
            {
                const float value = s_Samples[i][index];
                total += value;
                if (value > result.max) result.max = value;
            }
            result.average = total / static_cast<float>(s_Filled);
            return result;
        }

    private:
        inline static std::array<float, Count> s_Current{};
        inline static std::array<std::array<float, Count>, History> s_Samples{};
        inline static std::size_t s_Next = 0;
        inline static std::size_t s_Filled = 0;
    };

    class ProfileZone
    {
    public:
        explicit ProfileZone(FrameProfiler::Zone zone)
            : m_Zone(zone), m_Start(std::chrono::steady_clock::now()) {}
        ~ProfileZone()
        {
            const float ms = std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - m_Start).count();
            FrameProfiler::Record(m_Zone, ms);
        }
        ProfileZone(const ProfileZone&) = delete;
        ProfileZone& operator=(const ProfileZone&) = delete;

    private:
        FrameProfiler::Zone m_Zone;
        std::chrono::steady_clock::time_point m_Start;
    };
}

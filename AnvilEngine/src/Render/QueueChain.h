#pragma once
#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "../Core/Reference.h"
#include "RenderFrameCtx.h"

namespace anv
{
    class CommandBuffer;

    class QueueChain {
    public:
        using Task = std::function<void(Ref<CommandBuffer>, const RenderFrameContext&)>;
        using CmdQueue = std::queue<Task>;
        // Called AFTER End() when the batch is done (submit/present sync handled outside tasks)
        using SubmitFn = std::function<void(Ref<CommandBuffer>)>;

        QueueChain();
        ~QueueChain();

        void Start();                  // starts thread
        void Stop();

        void WriteToBack(Task task);
        void Swap();
        void WaitForProcessComplete();

        // New: clear both queues (or just back) safely
        void ClearBack();
        void ClearAll();

        // New: “hard stop” for swapchain recreation:
        // - Wait for any in-flight processing
        // - Clear any queued work that might reference old swapchain
        void Flush();

        // Provide the active command buffer used by the processing thread
        void SetActiveCommandBuffer(Ref<CommandBuffer> cmd);
        void SetActiveFrame(const RenderFrameContext& f);
        void SetActiveFrameSyncIndex(uint32_t idx);

        // Submit hook (VulkanRenderAPI sets this each frame)
        void SetSubmitFn(SubmitFn fn);

    private:
        std::unique_ptr<CmdQueue> m_Front;
        std::unique_ptr<CmdQueue> m_Back;

        std::thread m_ProcThread;

        std::mutex m_Mutex;
        std::condition_variable m_CV;

        std::atomic<bool> m_StopProc{ false };
        std::atomic<bool> m_ThreadRunning{ false };

        std::atomic<bool> m_WorkAvailable{ false };
        std::atomic<bool> m_ProcComplete{ true };

        Ref<CommandBuffer> m_ActiveCmd = nullptr; // owned elsewhere, used on render thread
        RenderFrameContext m_ActiveFrame{};
        
        uint32_t m_ActiveFrameSyncIndex = 0;

        SubmitFn m_SubmitFn;


        void ProcessFrontQueue();
    };
}

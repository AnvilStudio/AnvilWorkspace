#include "QueueChain.h"
#include "../Util/UMacros.h"
#include "CommandBuffer.h" 

namespace anv
{
    QueueChain::QueueChain() {
        m_Front = std::make_unique<CmdQueue>();
        m_Back = std::make_unique<CmdQueue>();
    }

    QueueChain::~QueueChain() {
        Stop();
    }

    void QueueChain::SetActiveCommandBuffer(Ref<CommandBuffer> cmd)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ActiveCmd = cmd;
    }

    void QueueChain::Start() {
        m_StopProc.store(false, std::memory_order_release);
        m_ThreadRunning.store(true, std::memory_order_release);
        m_ProcThread = std::thread(&QueueChain::ProcessFrontQueue, this);
    }

    void QueueChain::Stop() {
        if (!m_ThreadRunning.load(std::memory_order_acquire))
            return;

        m_StopProc.store(true, std::memory_order_release);
        m_CV.notify_one();

        if (m_ProcThread.joinable())
            m_ProcThread.join();

        m_ThreadRunning.store(false, std::memory_order_release);

        ClearAll();
    }

    void QueueChain::WriteToBack(Task task) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Back->push(std::move(task));
    }

    void QueueChain::Swap() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Back->empty())
            return;

        std::swap(m_Front, m_Back);
        m_WorkAvailable.store(true, std::memory_order_release);
        m_ProcComplete.store(false, std::memory_order_release);
        m_CV.notify_one();
    }

    void QueueChain::WaitForProcessComplete() {
        while (!m_ProcComplete.load(std::memory_order_acquire) &&
            !m_StopProc.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void QueueChain::ProcessFrontQueue()
    {
        while (!m_StopProc.load(std::memory_order_acquire))
        {
            // ---- wait for work ----
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_CV.wait(lock, [&] {
                return m_StopProc.load(std::memory_order_acquire) ||
                    m_WorkAvailable.load(std::memory_order_acquire);
                });

            if (m_StopProc.load(std::memory_order_acquire))
                break;

            // ---- snapshot batch state (ONE FRAME) ----
            Ref<CommandBuffer> cmd = m_ActiveCmd;
            RenderFrameContext frame = m_ActiveFrame;
            SubmitFn submitFn = m_SubmitFn;

            if (!cmd)
            {
                ANV_LOG_ERROR("[QueueChain] No active CommandBuffer set");
                m_WorkAvailable.store(false, std::memory_order_release);
                m_ProcComplete.store(true, std::memory_order_release);
                continue;
            }

            lock.unlock();

            // ---- record commands ----
            cmd->Begin();

            while (true)
            {
                Task task;

                {
                    std::lock_guard<std::mutex> qlock(m_Mutex);
                    if (m_Front->empty())
                        break;

                    task = std::move(m_Front->front());
                    m_Front->pop();
                }

                if (task)
                    task(cmd, frame);
            }

            cmd->End();

            // ---- submit (Vulkan-only logic via callback) ----
            if (submitFn)
                submitFn(cmd);

            // ---- mark batch complete ----
            {
                std::lock_guard<std::mutex> qlock(m_Mutex);
                m_WorkAvailable.store(false, std::memory_order_release);
                m_ProcComplete.store(true, std::memory_order_release);
            }
        }
    }


    void QueueChain::SetActiveFrame(const RenderFrameContext& f)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ActiveFrame = f;
    }

    void QueueChain::SetActiveFrameSyncIndex(uint32_t idx)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ActiveFrameSyncIndex = idx;
    }

    void QueueChain::SetSubmitFn(SubmitFn fn)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_SubmitFn = std::move(fn);
    }

    void QueueChain::ClearBack() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        CmdQueue empty;
        m_Back->swap(empty);
    }

    void QueueChain::ClearAll() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        CmdQueue empty;
        m_Back->swap(empty);
        m_Front->swap(empty);
    }

    void QueueChain::Flush() {
        // 1) wait for worker to finish whatever it is doing
        WaitForProcessComplete();

        // 2) clear any queued work (front should be empty if ProcComplete=true,
        // but clear it anyway to be safe)
        ClearAll();
    }
}
